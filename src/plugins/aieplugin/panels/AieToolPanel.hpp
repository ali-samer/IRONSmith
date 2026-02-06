#pragma once

#include <QtWidgets/QWidget>

namespace Utils {
class SidebarPanelFrame;
class ColorSwatchButton;
class LabeledSlider;
}

class QCheckBox;
class QGroupBox;

namespace Aie {
class AieCanvasCoordinator;
}

namespace Aie::Internal {

class AieToolPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit AieToolPanel(AieCanvasCoordinator* coordinator, QWidget* parent = nullptr);

private:
    void buildUi();
    void syncFromCoordinator();

    AieCanvasCoordinator* m_coordinator = nullptr;
    Utils::SidebarPanelFrame* m_frame = nullptr;
    Utils::LabeledSlider* m_horizontalSpacingSlider = nullptr;
    Utils::LabeledSlider* m_verticalSpacingSlider = nullptr;
    Utils::LabeledSlider* m_outwardSpreadSlider = nullptr;
    class QCheckBox* m_autoCellCheck = nullptr;
    Utils::LabeledSlider* m_cellSizeSlider = nullptr;
    class QCheckBox* m_showPortsCheck = nullptr;
    class QCheckBox* m_showLabelsCheck = nullptr;
    Utils::LabeledSlider* m_keepoutSlider = nullptr;
    class QCheckBox* m_useCustomColorsCheck = nullptr;
    Utils::ColorSwatchButton* m_fillColorButton = nullptr;
    Utils::ColorSwatchButton* m_outlineColorButton = nullptr;
    Utils::ColorSwatchButton* m_labelColorButton = nullptr;
};

} // namespace Aie::Internal
