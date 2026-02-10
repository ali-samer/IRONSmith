#include "aieplugin/panels/AieToolPanel.hpp"

#include "aieplugin/AieCanvasCoordinator.hpp"

#include <utils/ui/SidebarPanelFrame.hpp>
#include <utils/ui/ColorSwatchButton.hpp>
#include <utils/ui/LabeledSlider.hpp>

#include <QtCore/QSignalBlocker>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

#include <cmath>

namespace Aie::Internal {

namespace {

Utils::LabeledSlider* makeSlider(int min, int max, int step)
{
    auto* slider = new Utils::LabeledSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setSingleStep(step);
    slider->setPageStep(std::max(step * 4, 1));
    return slider;
}

} // namespace

AieToolPanel::AieToolPanel(AieCanvasCoordinator* coordinator, QWidget* parent)
    : QWidget(parent)
    , m_coordinator(coordinator)
{
    buildUi();
    syncFromCoordinator();
}

void AieToolPanel::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_frame = new Utils::SidebarPanelFrame(this);
    m_frame->setTitle(QStringLiteral("AIE Grid"));
    m_frame->setSearchEnabled(false);
    m_frame->setHeaderDividerVisible(true);

    auto* content = new QWidget(m_frame);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(12);

    auto* layoutGroup = new QGroupBox(QStringLiteral("Layout"), content);
    auto* layoutForm = new QFormLayout(layoutGroup);
    layoutForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layoutForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_horizontalSpacingSlider = makeSlider(0, 512, 1);
    m_verticalSpacingSlider = makeSlider(0, 512, 1);
    m_outwardSpreadSlider = makeSlider(0, 512, 1);
    m_autoCellCheck = new QCheckBox(QStringLiteral("Auto size"), layoutGroup);
    m_cellSizeSlider = makeSlider(24, 200, 2);

    layoutForm->addRow(QStringLiteral("Horizontal spacing"), m_horizontalSpacingSlider);
    layoutForm->addRow(QStringLiteral("Vertical spacing"), m_verticalSpacingSlider);
    layoutForm->addRow(QStringLiteral("Outward spread"), m_outwardSpreadSlider);
    layoutForm->addRow(QString(), m_autoCellCheck);
    layoutForm->addRow(QStringLiteral("Cell size"), m_cellSizeSlider);

    auto* selectionGroup = new QGroupBox(QStringLiteral("Selection"), content);
    auto* selectionLayout = new QVBoxLayout(selectionGroup);
    selectionLayout->setSpacing(6);
    auto* selectionForm = new QFormLayout();
    selectionForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    selectionForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_nudgeStepSlider = makeSlider(1, 64, 1);
    m_nudgeStepSlider->setValue(8);
    selectionForm->addRow(QStringLiteral("Nudge step"), m_nudgeStepSlider);

    auto* nudgeGrid = new QGridLayout();
    nudgeGrid->setHorizontalSpacing(6);
    nudgeGrid->setVerticalSpacing(6);
    m_nudgeUpButton = new QPushButton(QStringLiteral("Up"), selectionGroup);
    m_nudgeDownButton = new QPushButton(QStringLiteral("Down"), selectionGroup);
    m_nudgeLeftButton = new QPushButton(QStringLiteral("Left"), selectionGroup);
    m_nudgeRightButton = new QPushButton(QStringLiteral("Right"), selectionGroup);
    nudgeGrid->addWidget(m_nudgeUpButton, 0, 1);
    nudgeGrid->addWidget(m_nudgeLeftButton, 1, 0);
    nudgeGrid->addWidget(m_nudgeRightButton, 1, 2);
    nudgeGrid->addWidget(m_nudgeDownButton, 2, 1);

    selectionLayout->addLayout(selectionForm);
    selectionLayout->addLayout(nudgeGrid);

    auto* displayGroup = new QGroupBox(QStringLiteral("Display"), content);
    auto* displayForm = new QFormLayout(displayGroup);
    displayForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    displayForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_showPortsCheck = new QCheckBox(QStringLiteral("Show ports"), displayGroup);
    m_showLabelsCheck = new QCheckBox(QStringLiteral("Show labels"), displayGroup);
    m_keepoutSlider = makeSlider(-1, 40, 1);
    m_keepoutSlider->setSpecialValue(-1, QStringLiteral("Auto"));
    m_keepoutSlider->setValue(-1);

    displayForm->addRow(QString(), m_showPortsCheck);
    displayForm->addRow(QString(), m_showLabelsCheck);
    displayForm->addRow(QStringLiteral("Keepout"), m_keepoutSlider);

    auto* styleGroup = new QGroupBox(QStringLiteral("Style"), content);
    auto* styleForm = new QFormLayout(styleGroup);
    styleForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    styleForm->setFormAlignment(Qt::AlignTop | Qt::AlignLeft);

    m_useCustomColorsCheck = new QCheckBox(QStringLiteral("Custom colors"), styleGroup);
    m_fillColorButton = new Utils::ColorSwatchButton(styleGroup);
    m_outlineColorButton = new Utils::ColorSwatchButton(styleGroup);
    m_labelColorButton = new Utils::ColorSwatchButton(styleGroup);

    styleForm->addRow(QString(), m_useCustomColorsCheck);
    styleForm->addRow(QStringLiteral("Fill"), m_fillColorButton);
    styleForm->addRow(QStringLiteral("Outline"), m_outlineColorButton);
    styleForm->addRow(QStringLiteral("Label"), m_labelColorButton);

    layout->addWidget(layoutGroup);
    layout->addWidget(selectionGroup);
    layout->addWidget(displayGroup);
    layout->addWidget(styleGroup);
    layout->addStretch(1);

    m_frame->setContentWidget(content);
    root->addWidget(m_frame);

    if (!m_coordinator)
        return;

    connect(m_horizontalSpacingSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->setHorizontalSpacing(static_cast<double>(value));
            });
    connect(m_horizontalSpacingSlider, &Utils::LabeledSlider::sliderPressed,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->beginSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Horizontal);
            });
    connect(m_horizontalSpacingSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->updateSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Horizontal,
                                                          static_cast<double>(value));
            });
    connect(m_horizontalSpacingSlider, &Utils::LabeledSlider::sliderReleased,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->endSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Horizontal);
            });
    connect(m_verticalSpacingSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->setVerticalSpacing(static_cast<double>(value));
            });
    connect(m_verticalSpacingSlider, &Utils::LabeledSlider::sliderPressed,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->beginSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Vertical);
            });
    connect(m_verticalSpacingSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->updateSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Vertical,
                                                          static_cast<double>(value));
            });
    connect(m_verticalSpacingSlider, &Utils::LabeledSlider::sliderReleased,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->endSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Vertical);
            });
    connect(m_outwardSpreadSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->setOutwardSpread(static_cast<double>(value));
            });
    connect(m_outwardSpreadSlider, &Utils::LabeledSlider::sliderPressed,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->beginSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Outward);
            });
    connect(m_outwardSpreadSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->updateSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Outward,
                                                          static_cast<double>(value));
            });
    connect(m_outwardSpreadSlider, &Utils::LabeledSlider::sliderReleased,
            this, [this]() {
                if (m_coordinator)
                    m_coordinator->endSelectionSpacing(AieCanvasCoordinator::SelectionSpacingAxis::Outward);
            });
    connect(m_nudgeUpButton, &QPushButton::clicked, this, [this]() {
        if (!m_coordinator)
            return;
        const double step = static_cast<double>(m_nudgeStepSlider->value());
        m_coordinator->nudgeSelection(0.0, -step);
    });
    connect(m_nudgeDownButton, &QPushButton::clicked, this, [this]() {
        if (!m_coordinator)
            return;
        const double step = static_cast<double>(m_nudgeStepSlider->value());
        m_coordinator->nudgeSelection(0.0, step);
    });
    connect(m_nudgeLeftButton, &QPushButton::clicked, this, [this]() {
        if (!m_coordinator)
            return;
        const double step = static_cast<double>(m_nudgeStepSlider->value());
        m_coordinator->nudgeSelection(-step, 0.0);
    });
    connect(m_nudgeRightButton, &QPushButton::clicked, this, [this]() {
        if (!m_coordinator)
            return;
        const double step = static_cast<double>(m_nudgeStepSlider->value());
        m_coordinator->nudgeSelection(step, 0.0);
    });
    connect(m_autoCellCheck, &QCheckBox::toggled,
            m_coordinator, &AieCanvasCoordinator::setAutoCellSize);
    connect(m_cellSizeSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->setCellSize(static_cast<double>(value));
            });
    connect(m_showPortsCheck, &QCheckBox::toggled,
            m_coordinator, &AieCanvasCoordinator::setShowPorts);
    connect(m_showLabelsCheck, &QCheckBox::toggled,
            m_coordinator, &AieCanvasCoordinator::setShowLabels);
    connect(m_keepoutSlider, &Utils::LabeledSlider::valueChanged,
            this, [this](int value) {
                if (m_coordinator)
                    m_coordinator->setKeepoutMargin(static_cast<double>(value));
            });
    connect(m_useCustomColorsCheck, &QCheckBox::toggled,
            m_coordinator, &AieCanvasCoordinator::setUseCustomColors);
    connect(m_fillColorButton, &Utils::ColorSwatchButton::colorChanged, this,
            [this](const QColor& color) {
                if (m_coordinator) {
                    m_coordinator->setUseCustomColors(true);
                    m_coordinator->setFillColor(color);
                }
            });
    connect(m_outlineColorButton, &Utils::ColorSwatchButton::colorChanged, this,
            [this](const QColor& color) {
                if (m_coordinator) {
                    m_coordinator->setUseCustomColors(true);
                    m_coordinator->setOutlineColor(color);
                }
            });
    connect(m_labelColorButton, &Utils::ColorSwatchButton::colorChanged, this,
            [this](const QColor& color) {
                if (m_coordinator) {
                    m_coordinator->setUseCustomColors(true);
                    m_coordinator->setLabelColor(color);
                }
            });

    connect(m_coordinator, &AieCanvasCoordinator::autoCellSizeChanged, this,
            [this](bool enabled) { m_cellSizeSlider->setEnabled(!enabled); });

    connect(m_coordinator, &AieCanvasCoordinator::horizontalSpacingChanged, this,
            [this](double value) {
                QSignalBlocker block(m_horizontalSpacingSlider);
                m_horizontalSpacingSlider->setValue(static_cast<int>(std::lround(value)));
            });
    connect(m_coordinator, &AieCanvasCoordinator::verticalSpacingChanged, this,
            [this](double value) {
                QSignalBlocker block(m_verticalSpacingSlider);
                m_verticalSpacingSlider->setValue(static_cast<int>(std::lround(value)));
            });
    connect(m_coordinator, &AieCanvasCoordinator::outwardSpreadChanged, this,
            [this](double value) {
                QSignalBlocker block(m_outwardSpreadSlider);
                m_outwardSpreadSlider->setValue(static_cast<int>(std::lround(value)));
            });
    connect(m_coordinator, &AieCanvasCoordinator::autoCellSizeChanged, this,
            [this](bool enabled) {
                QSignalBlocker block(m_autoCellCheck);
                m_autoCellCheck->setChecked(enabled);
            });
    connect(m_coordinator, &AieCanvasCoordinator::cellSizeChanged, this,
            [this](double value) {
                QSignalBlocker block(m_cellSizeSlider);
                m_cellSizeSlider->setValue(static_cast<int>(std::lround(value)));
            });
    connect(m_coordinator, &AieCanvasCoordinator::showPortsChanged, this,
            [this](bool enabled) {
                QSignalBlocker block(m_showPortsCheck);
                m_showPortsCheck->setChecked(enabled);
            });
    connect(m_coordinator, &AieCanvasCoordinator::showLabelsChanged, this,
            [this](bool enabled) {
                QSignalBlocker block(m_showLabelsCheck);
                m_showLabelsCheck->setChecked(enabled);
            });
    connect(m_coordinator, &AieCanvasCoordinator::keepoutMarginChanged, this,
            [this](double value) {
                QSignalBlocker block(m_keepoutSlider);
                m_keepoutSlider->setValue(static_cast<int>(std::lround(value)));
            });
    connect(m_coordinator, &AieCanvasCoordinator::useCustomColorsChanged, this,
            [this](bool enabled) {
                QSignalBlocker block(m_useCustomColorsCheck);
                m_useCustomColorsCheck->setChecked(enabled);
                m_fillColorButton->setEnabled(enabled);
                m_outlineColorButton->setEnabled(enabled);
                m_labelColorButton->setEnabled(enabled);
            });
    connect(m_coordinator, &AieCanvasCoordinator::fillColorChanged, this,
            [this](const QColor& color) {
                QSignalBlocker block(m_fillColorButton);
                m_fillColorButton->setColor(color);
            });
    connect(m_coordinator, &AieCanvasCoordinator::outlineColorChanged, this,
            [this](const QColor& color) {
                QSignalBlocker block(m_outlineColorButton);
                m_outlineColorButton->setColor(color);
            });
    connect(m_coordinator, &AieCanvasCoordinator::labelColorChanged, this,
            [this](const QColor& color) {
                QSignalBlocker block(m_labelColorButton);
                m_labelColorButton->setColor(color);
            });
}

void AieToolPanel::syncFromCoordinator()
{
    if (!m_coordinator)
        return;

    QSignalBlocker blockHSpacing(m_horizontalSpacingSlider);
    QSignalBlocker blockVSpacing(m_verticalSpacingSlider);
    QSignalBlocker blockOutward(m_outwardSpreadSlider);
    QSignalBlocker blockAuto(m_autoCellCheck);
    QSignalBlocker blockCell(m_cellSizeSlider);
    QSignalBlocker blockPorts(m_showPortsCheck);
    QSignalBlocker blockLabels(m_showLabelsCheck);
    QSignalBlocker blockKeepout(m_keepoutSlider);
    QSignalBlocker blockCustom(m_useCustomColorsCheck);
    QSignalBlocker blockFill(m_fillColorButton);
    QSignalBlocker blockOutline(m_outlineColorButton);
    QSignalBlocker blockLabel(m_labelColorButton);

    m_horizontalSpacingSlider->setValue(static_cast<int>(std::lround(m_coordinator->horizontalSpacing())));
    m_verticalSpacingSlider->setValue(static_cast<int>(std::lround(m_coordinator->verticalSpacing())));
    m_outwardSpreadSlider->setValue(static_cast<int>(std::lround(m_coordinator->outwardSpread())));
    m_autoCellCheck->setChecked(m_coordinator->autoCellSize());
    m_cellSizeSlider->setValue(static_cast<int>(std::lround(m_coordinator->cellSize())));
    m_cellSizeSlider->setEnabled(!m_coordinator->autoCellSize());
    m_showPortsCheck->setChecked(m_coordinator->showPorts());
    m_showLabelsCheck->setChecked(m_coordinator->showLabels());
    m_keepoutSlider->setValue(static_cast<int>(std::lround(m_coordinator->keepoutMargin())));
    m_useCustomColorsCheck->setChecked(m_coordinator->useCustomColors());
    m_fillColorButton->setColor(m_coordinator->fillColor());
    m_outlineColorButton->setColor(m_coordinator->outlineColor());
    m_labelColorButton->setColor(m_coordinator->labelColor());
    const bool colorsEnabled = m_coordinator->useCustomColors();
    m_fillColorButton->setEnabled(colorsEnabled);
    m_outlineColorButton->setEnabled(colorsEnabled);
    m_labelColorButton->setEnabled(colorsEnabled);
}

} // namespace Aie::Internal
