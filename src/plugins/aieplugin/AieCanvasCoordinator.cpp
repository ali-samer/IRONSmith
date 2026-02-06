#include "aieplugin/AieCanvasCoordinator.hpp"

#include "aieplugin/AieConstants.hpp"

#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/CanvasConstants.hpp"

#include <QtCore/QMarginsF>
#include <QtCore/QtGlobal>

#include <algorithm>

namespace Aie {

namespace {

constexpr int kApplyDebounceMs = 50;

} // namespace

AieCanvasCoordinator::AieCanvasCoordinator(QObject* parent)
    : QObject(parent)
    , m_applyDebounce(this)
    , m_tileSpacing(Aie::kDefaultTileSpacing)
    , m_outerMargin(Aie::kDefaultOuterMargin)
    , m_autoCellSize(true)
    , m_cellSize(Aie::kDefaultCellSize)
    , m_showPorts(true)
    , m_showLabels(true)
    , m_keepoutMargin(Aie::kDefaultKeepoutMargin)
    , m_useCustomColors(false)
    , m_fillColor(QColor(Canvas::Constants::kBlockFillColor))
    , m_outlineColor(QColor(Canvas::Constants::kBlockOutlineColor))
    , m_labelColor(QColor(Canvas::Constants::kBlockTextColor))
{
    m_applyDebounce.setDelayMs(kApplyDebounceMs);
    m_applyDebounce.setAction([this]() { applyNow(); });
}

void AieCanvasCoordinator::setGridHost(Canvas::Api::ICanvasGridHost* host)
{
    if (m_gridHost == host)
        return;
    m_gridHost = host;
    requestApply();
}

void AieCanvasCoordinator::setStyleHost(Canvas::Api::ICanvasStyleHost* host)
{
    if (m_styleHost == host)
        return;
    m_styleHost = host;
    requestApply();
}

void AieCanvasCoordinator::setBaseModel(const CanvasGridModel& model)
{
    m_baseModel = model;
    requestApply();
}

void AieCanvasCoordinator::setBaseStyles(const QHash<QString, Canvas::Api::CanvasBlockStyle>& styles)
{
    m_baseStyles = styles;
    requestApply();
}

void AieCanvasCoordinator::setTileSpacing(double spacing)
{
    spacing = std::max(0.0, spacing);
    if (qFuzzyCompare(m_tileSpacing, spacing))
        return;
    m_tileSpacing = spacing;
    emit tileSpacingChanged(m_tileSpacing);
    requestApply();
}

void AieCanvasCoordinator::setOuterMargin(double margin)
{
    margin = std::max(0.0, margin);
    if (qFuzzyCompare(m_outerMargin, margin))
        return;
    m_outerMargin = margin;
    emit outerMarginChanged(m_outerMargin);
    requestApply();
}

void AieCanvasCoordinator::setAutoCellSize(bool enabled)
{
    if (m_autoCellSize == enabled)
        return;
    m_autoCellSize = enabled;
    emit autoCellSizeChanged(m_autoCellSize);
    requestApply();
}

void AieCanvasCoordinator::setCellSize(double size)
{
    size = std::max(1.0, size);
    if (qFuzzyCompare(m_cellSize, size))
        return;
    m_cellSize = size;
    emit cellSizeChanged(m_cellSize);
    if (!m_autoCellSize)
        requestApply();
}

void AieCanvasCoordinator::setShowPorts(bool enabled)
{
    if (m_showPorts == enabled)
        return;
    m_showPorts = enabled;
    emit showPortsChanged(m_showPorts);
    requestApply();
}

void AieCanvasCoordinator::setShowLabels(bool enabled)
{
    if (m_showLabels == enabled)
        return;
    m_showLabels = enabled;
    emit showLabelsChanged(m_showLabels);
    requestApply();
}

void AieCanvasCoordinator::setKeepoutMargin(double margin)
{
    if (qFuzzyCompare(m_keepoutMargin, margin))
        return;
    m_keepoutMargin = margin;
    emit keepoutMarginChanged(m_keepoutMargin);
    requestApply();
}

void AieCanvasCoordinator::setUseCustomColors(bool enabled)
{
    if (m_useCustomColors == enabled)
        return;
    m_useCustomColors = enabled;
    emit useCustomColorsChanged(m_useCustomColors);
    requestApply();
}

void AieCanvasCoordinator::setFillColor(const QColor& color)
{
    if (m_fillColor == color)
        return;
    m_fillColor = color;
    emit fillColorChanged(m_fillColor);
    if (m_useCustomColors)
        requestApply();
}

void AieCanvasCoordinator::setOutlineColor(const QColor& color)
{
    if (m_outlineColor == color)
        return;
    m_outlineColor = color;
    emit outlineColorChanged(m_outlineColor);
    if (m_useCustomColors)
        requestApply();
}

void AieCanvasCoordinator::setLabelColor(const QColor& color)
{
    if (m_labelColor == color)
        return;
    m_labelColor = color;
    emit labelColorChanged(m_labelColor);
    if (m_useCustomColors)
        requestApply();
}

void AieCanvasCoordinator::apply()
{
    if (!m_gridHost || !m_baseModel.gridSpec.isValid())
        return;

    Utils::GridSpec spec = m_baseModel.gridSpec;
    spec.cellSpacing = QSizeF(m_tileSpacing, m_tileSpacing);
    spec.outerMargin = QMarginsF(m_outerMargin, m_outerMargin, m_outerMargin, m_outerMargin);
    spec.autoCellSize = m_autoCellSize;
    spec.cellSize = m_autoCellSize ? QSizeF() : QSizeF(m_cellSize, m_cellSize);

    QVector<Canvas::Api::CanvasBlockSpec> blocks = m_baseModel.blocks;
    for (auto& block : blocks) {
        block.showPorts = m_showPorts;
        block.label = m_showLabels ? block.label : QString();
        if (m_keepoutMargin >= 0.0)
            block.keepoutMargin = m_keepoutMargin;
        else
            block.keepoutMargin = -1.0;

        if (!m_styleHost) {
            if (m_useCustomColors) {
                block.hasCustomColors = true;
                block.fillColor = m_fillColor;
                block.outlineColor = m_outlineColor;
                block.labelColor = m_labelColor;
            } else {
                block.hasCustomColors = false;
            }
        }
    }

    m_gridHost->setGridSpec(spec);
    m_gridHost->setBlocks(blocks);

    if (m_styleHost && !m_baseStyles.isEmpty()) {
        for (auto it = m_baseStyles.begin(); it != m_baseStyles.end(); ++it) {
            Canvas::Api::CanvasBlockStyle style = it.value();
            if (m_useCustomColors) {
                style.fillColor = m_fillColor;
                style.outlineColor = m_outlineColor;
                style.labelColor = m_labelColor;
            }
            m_styleHost->setBlockStyle(it.key(), style);
        }
    }
}

void AieCanvasCoordinator::requestApply()
{
    m_dirty = true;
    m_applyDebounce.trigger();
}

void AieCanvasCoordinator::applyNow()
{
    if (!m_dirty)
        return;
    if (!m_gridHost || !m_baseModel.gridSpec.isValid())
        return;
    m_dirty = false;
    apply();
}

} // namespace Aie
