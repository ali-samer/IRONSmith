#pragma once

#include "canvas/api/ICanvasHost.hpp"

#include <QtCore/QPointer>
#include <memory>

namespace ExtensionSystem {
class PluginManager;
}

namespace Core {
class IUiHost;
class StatusBarField;
}

namespace Canvas {

class CanvasDocument;
class CanvasController;
class CanvasView;
class CanvasSelectionModel;

namespace Internal {

class CanvasHostImpl final : public Canvas::Api::ICanvasHost
{
	Q_OBJECT

public:
	explicit CanvasHostImpl(QObject* parent = nullptr);

	void wireIntoApplication(ExtensionSystem::PluginManager& manager);

	QWidget* viewWidget() const override;
	CanvasDocument* document() const override;
	CanvasController* controller() const override;

private:
	QPointer<Core::IUiHost> m_uiHost;
	QPointer<Core::StatusBarField> m_modeField;
	QPointer<Core::StatusBarField> m_zoomField;
	QPointer<Core::StatusBarField> m_panField;
	QPointer<Core::StatusBarField> m_selectionField;

	QPointer<CanvasDocument> m_document;
	QPointer<CanvasController> m_controller;
	QPointer<CanvasView> m_view;
	QPointer<CanvasSelectionModel> m_selection;
};

} // namespace Internal
} // namespace Canvas
