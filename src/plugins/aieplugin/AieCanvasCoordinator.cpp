#include "aieplugin/AieCanvasCoordinator.hpp"

#include "aieplugin/AieConstants.hpp"

#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/api/ICanvasHost.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasController.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasCommands.hpp"

#include <QtCore/QMarginsF>
#include <QtCore/QtGlobal>
#include <QtCore/QSet>

#include <algorithm>
#include <limits>
#include <numeric>

namespace Aie {

namespace {

constexpr int kApplyDebounceMs = 50;

struct SelectionBlockInfo final {
    Canvas::ObjectId id{};
    QRectF bounds;

    QPointF topLeft() const { return bounds.topLeft(); }
    QSizeF size() const { return bounds.size(); }
    QPointF center() const { return bounds.center(); }
};

struct SelectionLayout final {
    QVector<SelectionBlockInfo> blocks;
    QRectF bounds;
    QPointF center;
};

} // namespace

struct AieCanvasCoordinator::SelectionSnapshot final {
    SelectionSpacingAxis axis = SelectionSpacingAxis::Horizontal;
    SelectionLayout layout;
};

namespace {

SelectionLayout buildSelectionLayout(Canvas::CanvasDocument* doc, const QSet<Canvas::ObjectId>& ids)
{
    SelectionLayout layout;
    if (!doc)
        return layout;

    for (const auto& id : ids) {
        auto* item = doc->findItem(id);
        auto* block = dynamic_cast<Canvas::CanvasBlock*>(item);
        if (!block || !block->isMovable())
            continue;

        SelectionBlockInfo info;
        info.id = id;
        info.bounds = block->boundsScene();
        layout.blocks.push_back(info);
        if (layout.bounds.isNull())
            layout.bounds = info.bounds;
        else
            layout.bounds = layout.bounds.united(info.bounds);
    }

    if (!layout.blocks.isEmpty())
        layout.center = layout.bounds.center();
    return layout;
}

double minBlockExtent(const QVector<SelectionBlockInfo>& blocks, bool horizontal)
{
    double minExtent = std::numeric_limits<double>::max();
    for (const auto& block : blocks) {
        const double extent = horizontal ? block.bounds.width() : block.bounds.height();
        minExtent = std::min(minExtent, extent);
    }
    if (minExtent == std::numeric_limits<double>::max())
        return 0.0;
    return minExtent;
}

QVector<QVector<int>> clusterByAxis(const QVector<SelectionBlockInfo>& blocks, bool byY)
{
    QVector<int> order;
    order.reserve(blocks.size());
    for (int i = 0; i < blocks.size(); ++i)
        order.push_back(i);

    std::sort(order.begin(), order.end(), [&](int a, int b) {
        const QPointF ca = blocks[a].center();
        const QPointF cb = blocks[b].center();
        return byY ? (ca.y() < cb.y()) : (ca.x() < cb.x());
    });

    const double minExtent = minBlockExtent(blocks, !byY);
    const double tolerance = std::max(1.0, minExtent * 0.5);

    QVector<QVector<int>> groups;
    double groupCenter = 0.0;
    int groupCount = 0;

    for (int idx : order) {
        const double pos = byY ? blocks[idx].center().y() : blocks[idx].center().x();
        if (groups.isEmpty() || std::abs(pos - groupCenter) > tolerance) {
            groups.push_back(QVector<int>{idx});
            groupCenter = pos;
            groupCount = 1;
        } else {
            groups.back().push_back(idx);
            groupCenter = (groupCenter * groupCount + pos) / static_cast<double>(groupCount + 1);
            ++groupCount;
        }
    }
    return groups;
}

QHash<Canvas::ObjectId, QPointF> computeHorizontalSpacing(const SelectionLayout& layout, double spacing)
{
    QHash<Canvas::ObjectId, QPointF> targets;
    if (layout.blocks.isEmpty())
        return targets;

    const auto rows = clusterByAxis(layout.blocks, true);
    for (const auto& row : rows) {
        if (row.isEmpty())
            continue;
        QVector<int> ordered = row;
        std::sort(ordered.begin(), ordered.end(), [&](int a, int b) {
            return layout.blocks[a].center().x() < layout.blocks[b].center().x();
        });

        double totalWidth = 0.0;
        for (int idx : ordered)
            totalWidth += layout.blocks[idx].bounds.width();
        const qsizetype gaps = std::max<qsizetype>(0, ordered.size() - 1);
        totalWidth += spacing * static_cast<double>(gaps);

        double centerX = 0.0;
        for (int idx : ordered)
            centerX += layout.blocks[idx].center().x();
        centerX /= static_cast<double>(ordered.size());

        double left = centerX - totalWidth * 0.5;
        for (int idx : ordered) {
            const auto& block = layout.blocks[idx];
            QPointF topLeft(left, block.bounds.top());
            targets.insert(block.id, topLeft);
            left += block.bounds.width() + spacing;
        }
    }
    return targets;
}

QHash<Canvas::ObjectId, QPointF> computeVerticalSpacing(const SelectionLayout& layout, double spacing)
{
    QHash<Canvas::ObjectId, QPointF> targets;
    if (layout.blocks.isEmpty())
        return targets;

    const auto cols = clusterByAxis(layout.blocks, false);
    for (const auto& col : cols) {
        if (col.isEmpty())
            continue;
        QVector<int> ordered = col;
        std::sort(ordered.begin(), ordered.end(), [&](int a, int b) {
            return layout.blocks[a].center().y() < layout.blocks[b].center().y();
        });

        double totalHeight = 0.0;
        for (int idx : ordered)
            totalHeight += layout.blocks[idx].bounds.height();
        const qsizetype gaps = std::max<qsizetype>(0, ordered.size() - 1);
        totalHeight += spacing * static_cast<double>(gaps);

        double centerY = 0.0;
        for (int idx : ordered)
            centerY += layout.blocks[idx].center().y();
        centerY /= static_cast<double>(ordered.size());

        double top = centerY - totalHeight * 0.5;
        for (int idx : ordered) {
            const auto& block = layout.blocks[idx];
            QPointF topLeft(block.bounds.left(), top);
            targets.insert(block.id, topLeft);
            top += block.bounds.height() + spacing;
        }
    }
    return targets;
}

QHash<Canvas::ObjectId, QPointF> computeOutwardSpread(const SelectionLayout& layout, double spread)
{
    QHash<Canvas::ObjectId, QPointF> targets;
    if (layout.blocks.isEmpty())
        return targets;

    const QRectF bounds = layout.bounds;
    const QPointF center = layout.center;
    const double width = bounds.width();
    const double height = bounds.height();

    double scaleX = 1.0;
    double scaleY = 1.0;
    if (width > 1e-3)
        scaleX = (width + 2.0 * spread) / width;
    if (height > 1e-3)
        scaleY = (height + 2.0 * spread) / height;

    for (const auto& block : layout.blocks) {
        const QPointF delta = block.center() - center;
        const QPointF newCenter(center.x() + delta.x() * scaleX,
                                center.y() + delta.y() * scaleY);
        const QPointF topLeft = newCenter - QPointF(block.bounds.width() * 0.5,
                                                    block.bounds.height() * 0.5);
        targets.insert(block.id, topLeft);
    }
    return targets;
}

} // namespace

AieCanvasCoordinator::AieCanvasCoordinator(QObject* parent)
    : QObject(parent)
    , m_applyDebounce(this)
    , m_horizontalSpacing(Aie::kDefaultTileSpacing)
    , m_verticalSpacing(Aie::kDefaultTileSpacing)
    , m_outwardSpread(Aie::kDefaultOuterMargin)
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

AieCanvasCoordinator::~AieCanvasCoordinator() = default;

void AieCanvasCoordinator::setCanvasHost(Canvas::Api::ICanvasHost* host)
{
    if (m_canvasHost == host)
        return;
    m_canvasHost = host;
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

double AieCanvasCoordinator::tileSpacing() const
{
    if (qFuzzyCompare(m_horizontalSpacing, m_verticalSpacing))
        return m_horizontalSpacing;
    return (m_horizontalSpacing + m_verticalSpacing) * 0.5;
}

void AieCanvasCoordinator::setTileSpacing(double spacing)
{
    setHorizontalSpacing(spacing);
    setVerticalSpacing(spacing);
}

void AieCanvasCoordinator::setHorizontalSpacing(double spacing)
{
    spacing = std::max(0.0, spacing);
    if (qFuzzyCompare(m_horizontalSpacing, spacing))
        return;
    m_horizontalSpacing = spacing;
    emit horizontalSpacingChanged(m_horizontalSpacing);
    emit tileSpacingChanged(tileSpacing());
    requestApply();
}

void AieCanvasCoordinator::setVerticalSpacing(double spacing)
{
    spacing = std::max(0.0, spacing);
    if (qFuzzyCompare(m_verticalSpacing, spacing))
        return;
    m_verticalSpacing = spacing;
    emit verticalSpacingChanged(m_verticalSpacing);
    emit tileSpacingChanged(tileSpacing());
    requestApply();
}

void AieCanvasCoordinator::setOutwardSpread(double spread)
{
    spread = std::max(0.0, spread);
    if (qFuzzyCompare(m_outwardSpread, spread))
        return;
    m_outwardSpread = spread;
    emit outwardSpreadChanged(m_outwardSpread);
    emit outerMarginChanged(m_outwardSpread);
    requestApply();
}

void AieCanvasCoordinator::setOuterMargin(double margin)
{
    setOutwardSpread(margin);
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
    spec.cellSpacing = QSizeF(m_horizontalSpacing, m_verticalSpacing);
    spec.outerMargin = QMarginsF(m_outwardSpread, m_outwardSpread, m_outwardSpread, m_outwardSpread);
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

void AieCanvasCoordinator::beginSelectionSpacing(SelectionSpacingAxis axis)
{
    auto* host = m_canvasHost.data();
    auto* doc = host ? host->document() : nullptr;
    auto* controller = host ? host->controller() : nullptr;
    if (!doc || !controller)
        return;

    SelectionLayout layout = buildSelectionLayout(doc, controller->selectedItems());
    if (layout.blocks.isEmpty())
        return;

    if ((axis == SelectionSpacingAxis::Horizontal || axis == SelectionSpacingAxis::Vertical) &&
        layout.blocks.size() < 2)
        return;

    m_selectionSnapshot = std::make_unique<SelectionSnapshot>();
    m_selectionSnapshot->axis = axis;
    m_selectionSnapshot->layout = std::move(layout);
}

void AieCanvasCoordinator::updateSelectionSpacing(SelectionSpacingAxis axis, double value)
{
    if (!m_selectionSnapshot || m_selectionSnapshot->axis != axis)
        return;

    auto* host = m_canvasHost.data();
    auto* doc = host ? host->document() : nullptr;
    if (!doc)
        return;

    const double spacing = std::max(0.0, value);
    QHash<Canvas::ObjectId, QPointF> targets;
    switch (axis) {
        case SelectionSpacingAxis::Horizontal:
            targets = computeHorizontalSpacing(m_selectionSnapshot->layout, spacing);
            break;
        case SelectionSpacingAxis::Vertical:
            targets = computeVerticalSpacing(m_selectionSnapshot->layout, spacing);
            break;
        case SelectionSpacingAxis::Outward:
            targets = computeOutwardSpread(m_selectionSnapshot->layout, spacing);
            break;
    }

    for (auto it = targets.constBegin(); it != targets.constEnd(); ++it)
        doc->previewSetItemTopLeft(it.key(), it.value());
}

void AieCanvasCoordinator::endSelectionSpacing(SelectionSpacingAxis axis)
{
    if (!m_selectionSnapshot || m_selectionSnapshot->axis != axis)
        return;

    auto* host = m_canvasHost.data();
    auto* doc = host ? host->document() : nullptr;
    if (!doc) {
        m_selectionSnapshot.reset();
        return;
    }

    auto batch = std::make_unique<Canvas::CompositeCommand>(QStringLiteral("Move Items"));
    for (const auto& block : m_selectionSnapshot->layout.blocks) {
        auto* item = doc->findItem(block.id);
        auto* blk = dynamic_cast<Canvas::CanvasBlock*>(item);
        if (!blk)
            continue;
        const QPointF finalTopLeft = blk->boundsScene().topLeft();
        if (finalTopLeft == block.topLeft())
            continue;
        batch->add(std::make_unique<Canvas::MoveItemCommand>(block.id, block.topLeft(), finalTopLeft));
    }
    if (!batch->empty())
        doc->commands().execute(std::move(batch));

    m_selectionSnapshot.reset();
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
