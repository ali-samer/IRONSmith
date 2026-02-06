#include "canvas/internal/CanvasHostImpl.hpp"

#include "canvas/CanvasController.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasBlockContent.hpp"
#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QSizeF>

#include <vector>

#include <extensionsystem/PluginManager.hpp>
#include <core/ui/IUiHost.hpp>
#include <core/StatusBarField.hpp>
#include <core/widgets/InfoBarWidget.hpp>

#include <QtCore/QString>

namespace Canvas::Internal {

CanvasHostImpl::CanvasHostImpl(QObject* parent)
	: Canvas::Api::ICanvasHost(parent)
{
}


static BlockContentStyle makeStyle(const QColor& fill, const QColor& outline, const QColor& text)
{
    BlockContentStyle style;
    style.fill = fill;
    style.outline = outline;
    style.text = text;
    return style;
}

static std::unique_ptr<BlockContent> makeCompositeContent()
{
    auto root = std::make_unique<BlockContentContainer>(BlockContentContainer::Layout::Vertical);
    root->setGap(8.0);

    auto core = std::make_unique<BlockContentBlock>(
        QStringLiteral("Compute\nCore"),
        makeStyle(QColor("#1E6F3B"), QColor("#3ACC74"), QColor("#E5FFE9")));
    core->setPreferredSize(QSizeF(120.0, 44.0));
    root->addChild(std::move(core));

    auto row = std::make_unique<BlockContentContainer>(BlockContentContainer::Layout::Horizontal);
    row->setGap(8.0);

    auto l1 = std::make_unique<BlockContentBlock>(
        QStringLiteral("L1\nMem"),
        makeStyle(QColor("#3E6FC2"), QColor("#6FA6FF"), QColor("#ECF4FF")));
    l1->setPreferredSize(QSizeF(60.0, 36.0));
    row->addChild(std::move(l1));

    auto dma = std::make_unique<BlockContentBlock>(
        QStringLiteral("DMA"),
        makeStyle(QColor("#5E3B9E"), QColor("#9A78E8"), QColor("#F1E9FF")));
    dma->setPreferredSize(QSizeF(60.0, 36.0));
    row->addChild(std::move(dma));

    root->addChild(std::move(row));
    return root;
}

static void createDemo(CanvasDocument* document)
{
    if (!document)
        return;

    const auto makePorts = []() -> std::vector<CanvasPort> {
        std::vector<CanvasPort> ports;
        ports.push_back(CanvasPort{PortId::create(), PortRole::Dynamic, PortSide::Left, 0.50, QStringLiteral("D0")});
        ports.push_back(CanvasPort{PortId::create(), PortRole::Dynamic, PortSide::Right, 0.25, QStringLiteral("D1")});
        ports.push_back(CanvasPort{PortId::create(), PortRole::Dynamic, PortSide::Right, 0.40, QStringLiteral("D2")});
        ports.push_back(CanvasPort{PortId::create(), PortRole::Dynamic, PortSide::Right, 0.55, QStringLiteral("D3")});
        ports.push_back(CanvasPort{PortId::create(), PortRole::Dynamic, PortSide::Right, 0.70, QStringLiteral("D4")});
        return ports;
    };

    if (auto* blk = document->createBlock(QRectF(QPointF(64.0, 64.0), QSizeF(180.0, 120.0)), true)) {
        blk->setLabel(QString());
        blk->setPorts(makePorts());
        blk->setContent(makeCompositeContent());
    }

    if (auto* blk = document->createBlock(QRectF(QPointF(320.0, 96.0), QSizeF(160.0, 96.0)), true)) {
        blk->setLabel(QStringLiteral("AIE"));
        blk->setPorts(makePorts());
    }
}

void CanvasHostImpl::wireIntoApplication(ExtensionSystem::PluginManager& manager)
{
	if (m_view)
		return;

	m_uiHost = manager.getObject<Core::IUiHost>();
	if (!m_uiHost)
		return;

	m_document = new CanvasDocument(this);
	m_document->setStatusText(QStringLiteral("Canvas"));


	createDemo(m_document);

	m_view = new CanvasView();
	m_view->setDocument(m_document);

	m_controller = new CanvasController(m_document, m_view, this);
	m_view->setController(m_controller);

	if (auto* bar = m_uiHost ? m_uiHost->playgroundBottomBar() : nullptr) {
		auto* mode = bar->ensureField(QStringLiteral("mode"));
		if (mode) {
			mode->setLabel(QString());
			mode->setSide(Core::StatusBarField::Side::Left);
			mode->setValue(QStringLiteral("NORMAL"));
			m_modeField = mode;
		}
	}

	auto modeLabel = [](CanvasController::Mode mode) -> QString {
		switch (mode) {
			case CanvasController::Mode::Normal:  return QStringLiteral("NORMAL");
			case CanvasController::Mode::Panning: return QStringLiteral("PANNING");
			case CanvasController::Mode::Linking: return QStringLiteral("LINKING");
		}
		return QStringLiteral("NORMAL");
	};
	auto linkingLabel = [](CanvasController::LinkingMode mode) -> QString {
		switch (mode) {
			case CanvasController::LinkingMode::Split: return QStringLiteral("SPLIT");
			case CanvasController::LinkingMode::Join: return QStringLiteral("JOIN");
			case CanvasController::LinkingMode::Broadcast: return QStringLiteral("BROADCAST");
			case CanvasController::LinkingMode::Normal: break;
		}
		return QString();
	};
	auto modeValue = [modeLabel, linkingLabel](CanvasController::Mode mode,
	                                           CanvasController::LinkingMode linkingMode) -> QString {
		const QString base = modeLabel(mode);
		if (mode != CanvasController::Mode::Linking)
			return base;
		const QString sub = linkingLabel(linkingMode);
		if (sub.isEmpty())
			return base;
		return base + QStringLiteral("|") + sub;
	};

	if (m_controller) {
		auto updateMode = [this, modeValue]() {
			if (m_modeField && m_controller)
				m_modeField->setValue(modeValue(m_controller->mode(), m_controller->linkingMode()));
		};
		connect(m_controller, &CanvasController::modeChanged, this,
		        [updateMode](CanvasController::Mode) { updateMode(); });
		connect(m_controller, &CanvasController::linkingModeChanged, this,
		        [updateMode](CanvasController::LinkingMode) { updateMode(); });
		updateMode();
	}

	connect(m_view, &CanvasView::canvasMousePressed, m_controller, &CanvasController::onCanvasMousePressed);
	connect(m_view, &CanvasView::canvasMouseMoved, m_controller, &CanvasController::onCanvasMouseMoved);
	connect(m_view, &CanvasView::canvasMouseReleased, m_controller, &CanvasController::onCanvasMouseReleased);
	connect(m_view, &CanvasView::canvasWheel, m_controller, &CanvasController::onCanvasWheel);
	connect(m_view, &CanvasView::canvasKeyPressed, m_controller, &CanvasController::onCanvasKeyPressed);

	m_uiHost->setPlaygroundCenterBase(m_view);
	m_view->setFocus(Qt::OtherFocusReason);
}

QWidget* CanvasHostImpl::viewWidget() const
{
	return m_view;
}

CanvasDocument* CanvasHostImpl::document() const
{
	return m_document;
}

CanvasController* CanvasHostImpl::controller() const
{
	return m_controller;
}

} // namespace Canvas::Internal
