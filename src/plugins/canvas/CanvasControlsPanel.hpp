#pragma once

#include <QtWidgets/QWidget>

class QSpinBox;
class QPushButton;
class QCheckBox;

namespace Command { class CommandDispatcher; }

namespace Canvas {

class CanvasService;

class CanvasControlsPanel final : public QWidget
{
    Q_OBJECT
public:
    CanvasControlsPanel(QWidget* parent, CanvasService* service, Command::CommandDispatcher* dispatcher);

private slots:
    void onPlaceCompute();
    void onToggleAnnotations(bool checked);
    void onToggleFabric(bool checked);
    void onTogglePortHotspots(bool checked);

private:
    CanvasService* m_service{nullptr};
    Command::CommandDispatcher* m_dispatcher{nullptr};

    QSpinBox* m_col{nullptr};
    QSpinBox* m_row{nullptr};
    QPushButton* m_place{nullptr};
    QCheckBox* m_showAnnotations{nullptr};
    QCheckBox* m_showFabric{nullptr};
    QCheckBox* m_showPortHotspots{nullptr};
};

} // namespace Canvas