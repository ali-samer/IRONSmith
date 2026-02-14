#pragma once

#include "canvas/api/ICanvasHost.hpp"

#include <QtCore/QPointer>
#include <QtCore/QString>
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

    void setCanvasActive(bool active) override;
    bool canvasActive() const override;
    void setEmptyStateText(const QString& title, const QString& message) override;

private:
    void applyEmptyState();

	QPointer<Core::IUiHost> m_uiHost;
	QPointer<Core::StatusBarField> m_modeField;
	QPointer<Core::StatusBarField> m_zoomField;
	QPointer<Core::StatusBarField> m_panField;
	QPointer<Core::StatusBarField> m_selectionField;

	QPointer<CanvasDocument> m_document;
	QPointer<CanvasController> m_controller;
	QPointer<CanvasView> m_view;
	QPointer<CanvasSelectionModel> m_selection;

    bool m_canvasActive = false;
    QString m_emptyTitle;
    QString m_emptyMessage;
};

} // namespace Internal
} // namespace Canvas
