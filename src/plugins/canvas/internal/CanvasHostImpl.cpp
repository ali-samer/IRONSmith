#include "canvas/internal/CanvasHostImpl.hpp"

#include "canvas/CanvasController.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"

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


void CanvasHostImpl::wireIntoApplication(ExtensionSystem::PluginManager& manager)
{
	if (m_view)
		return;

	m_uiHost = manager.getObject<Core::IUiHost>();
	if (!m_uiHost)
		return;

	m_document = new CanvasDocument(this);
	m_document->setStatusText(QStringLiteral("Canvas"));

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
