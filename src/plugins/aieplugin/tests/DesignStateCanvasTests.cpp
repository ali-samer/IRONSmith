// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "aieplugin/design/DesignStateCanvas.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasDocument.hpp"

#include <QtCore/QRectF>

using Aie::Internal::DesignState;

namespace {

Canvas::CanvasBlock* makeBlock(Canvas::CanvasDocument& doc, const QString& specId)
{
    auto* block = doc.createBlock(QRectF(0.0, 0.0, 64.0, 64.0), false);
    if (!block)
        return nullptr;
    block->setSpecId(specId);
    return block;
}

} // namespace

TEST(DesignStateCanvasTests, ApplyDesignStateClearsPorts)
{
    Canvas::CanvasDocument doc;
    auto* block = makeBlock(doc, QStringLiteral("shim0_0"));
    ASSERT_NE(block, nullptr);
    block->addPort(Canvas::PortSide::Left, 0.5, Canvas::PortRole::Consumer);
    ASSERT_TRUE(block->hasPorts());

    DesignState state;
    const Utils::Result result = Aie::Internal::applyDesignStateToCanvas(state, doc, nullptr);
    ASSERT_TRUE(result.ok) << result.errors.join("\n").toStdString();

    EXPECT_FALSE(block->hasPorts());
    EXPECT_TRUE(block->ports().empty());
}
