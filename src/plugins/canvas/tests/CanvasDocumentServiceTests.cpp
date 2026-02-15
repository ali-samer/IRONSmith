// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasController.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/api/ICanvasHost.hpp"
#include "canvas/document/CanvasDocumentServiceImpl.hpp"

#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>

namespace {

class StubCanvasHost final : public Canvas::Api::ICanvasHost
{
public:
    StubCanvasHost()
    {
        m_document = new Canvas::CanvasDocument(this);
    }

    QWidget* viewWidget() const override { return nullptr; }
    Canvas::CanvasDocument* document() const override { return m_document; }
    Canvas::CanvasController* controller() const override { return nullptr; }

    void setCanvasActive(bool active) override
    {
        if (m_active == active)
            return;
        m_active = active;
        emit canvasActiveChanged(m_active);
    }

    bool canvasActive() const override { return m_active; }
    void setEmptyStateText(const QString&, const QString&) override {}

private:
    Canvas::CanvasDocument* m_document = nullptr;
    bool m_active = false;
};

QCoreApplication* ensureCoreApp()
{
    if (auto* existing = QCoreApplication::instance())
        return existing;

    static int argc = 1;
    static char appName[] = "CanvasTests";
    static char* argv[] = {appName, nullptr};
    static QCoreApplication app(argc, argv);
    return &app;
}

} // namespace

TEST(CanvasDocumentServiceTests, CreateSaveCloseAndReopenRoundTrip)
{
    ensureCoreApp();

    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());

    const QString bundlePath = QDir(temp.path()).filePath(QStringLiteral("RoundTrip.ironsmith"));
    ASSERT_TRUE(QDir().mkpath(bundlePath));

    StubCanvasHost host;
    Canvas::Internal::CanvasDocumentServiceImpl service;
    service.setCanvasHost(&host);

    Canvas::Api::CanvasDocumentCreateRequest createRequest;
    createRequest.bundlePath = bundlePath;
    createRequest.persistenceRelativePath = QStringLiteral("canvas/document.json");

    Canvas::Api::CanvasDocumentHandle handle;
    const Utils::Result createResult = service.createDocument(createRequest, handle);
    ASSERT_TRUE(createResult.ok) << createResult.errors.join("\n").toStdString();
    ASSERT_TRUE(handle.isValid());
    EXPECT_TRUE(QFileInfo::exists(handle.persistencePath));

    auto* block = host.document()->createBlock(QRectF(100.0, 100.0, 40.0, 40.0), true);
    ASSERT_NE(block, nullptr);
    block->setSpecId(QStringLiteral("tile-0"));

    Canvas::PortRef ref;
    ref.itemId = block->id();
    ref.portId = block->addPort(Canvas::PortSide::Right, 0.5, Canvas::PortRole::Producer, QStringLiteral("OUT"));

    Canvas::CanvasWire::Endpoint a;
    a.attached = ref;
    a.freeScene = QPointF();

    Canvas::CanvasWire::Endpoint b;
    b.freeScene = QPointF(180.0, 120.0);

    auto wire = std::make_unique<Canvas::CanvasWire>(a, b);
    wire->setId(host.document()->allocateId());
    ASSERT_TRUE(host.document()->insertItem(host.document()->items().size(), std::move(wire)));

    const Utils::Result saveResult = service.saveDocument(handle);
    ASSERT_TRUE(saveResult.ok) << saveResult.errors.join("\n").toStdString();

    const Utils::Result closeResult = service.closeDocument(handle,
        Canvas::Api::CanvasDocumentCloseReason::UserClosed);
    ASSERT_TRUE(closeResult.ok) << closeResult.errors.join("\n").toStdString();
    EXPECT_FALSE(service.hasOpenDocument());

    Canvas::Api::CanvasDocumentOpenRequest openRequest;
    openRequest.bundlePath = bundlePath;
    openRequest.persistencePath = handle.persistencePath;

    Canvas::Api::CanvasDocumentHandle reopened;
    const Utils::Result openResult = service.openDocument(openRequest, reopened);
    ASSERT_TRUE(openResult.ok) << openResult.errors.join("\n").toStdString();
    EXPECT_TRUE(reopened.isValid());
    EXPECT_EQ(host.document()->items().size(), 2u);
}
