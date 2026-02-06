#include "canvas/internal/CanvasGridHostImpl.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/internal/CanvasBlockHandleImpl.hpp"
#include "canvas/api/ICanvasStyleHost.hpp"
#include "canvas/api/CanvasStyleTypes.hpp"

#include "utils/ui/GridLayout.hpp"

#include <QtCore/QEvent>

namespace Canvas::Internal {

namespace {

constexpr double kFallbackCellSize = Canvas::Constants::kGridStep * 10.0;

} // namespace

CanvasGridHostImpl::CanvasGridHostImpl(CanvasDocument* document,
                                       CanvasView* view,
                                       Canvas::Api::ICanvasStyleHost* styleHost,
                                       QObject* parent)
    : Canvas::Api::ICanvasGridHost(parent)
    , m_document(document)
    , m_view(view)
    , m_styleHost(styleHost)
{
    if (m_view)
        m_view->installEventFilter(this);
}

void CanvasGridHostImpl::setGridSpec(const Utils::GridSpec& spec)
{
    if (m_gridSpec.columns == spec.columns &&
        m_gridSpec.rows == spec.rows &&
        m_gridSpec.origin == spec.origin &&
        m_gridSpec.cellSize == spec.cellSize &&
        m_gridSpec.autoCellSize == spec.autoCellSize &&
        m_gridSpec.cellSpacing == spec.cellSpacing &&
        m_gridSpec.outerMargin == spec.outerMargin)
        return;

    m_gridSpec = spec;
    emit gridSpecChanged(m_gridSpec);
    rebuildBlocks();
}

Utils::GridSpec CanvasGridHostImpl::gridSpec() const
{
    return m_gridSpec;
}

void CanvasGridHostImpl::setBlocks(const QVector<Canvas::Api::CanvasBlockSpec>& blocks)
{
    m_blockSpecs = blocks;
    rebuildBlocks();
}

void CanvasGridHostImpl::clearBlocks()
{
    m_blockSpecs.clear();
    rebuildBlocks();
}

Canvas::Api::ICanvasBlockHandle* CanvasGridHostImpl::blockHandle(const QString& id) const
{
    return m_handles.value(id, nullptr);
}

QVector<Canvas::Api::ICanvasBlockHandle*> CanvasGridHostImpl::blockHandles() const
{
    QVector<Canvas::Api::ICanvasBlockHandle*> result;
    result.reserve(m_handles.size());
    for (auto* handle : m_handles)
        result.push_back(handle);
    return result;
}

bool CanvasGridHostImpl::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view && event && event->type() == QEvent::Resize) {
        if (m_gridSpec.autoCellSize)
            rebuildBlocks();
    }

    return Canvas::Api::ICanvasGridHost::eventFilter(watched, event);
}

void CanvasGridHostImpl::rebuildBlocks()
{
    removeAllBlocks();

    if (!m_document || !m_gridSpec.isValid())
        return;

    const QSizeF cellSize = resolveCellSize();

    QHash<QString, CanvasBlockHandleImpl*> nextHandles;
    nextHandles.reserve(m_blockSpecs.size());

    for (const auto& spec : m_blockSpecs) {
        if (spec.id.trimmed().isEmpty())
            continue;
        if (!spec.gridRect.isValid())
            continue;

        const QRectF gridRect = rectForBlock(spec, cellSize);
        const QSizeF size = spec.hasPreferredSize() ? spec.preferredSize : gridRect.size();
        const QRectF bounds(gridRect.topLeft(), size);

        CanvasBlock* block = m_document->createBlock(bounds, spec.movable);
        if (!block)
            continue;

        block->setLabel(spec.label);
        block->setShowPorts(spec.showPorts);
        block->setDeletable(spec.deletable);
        if (spec.keepoutMargin >= 0.0)
            block->setKeepoutMargin(spec.keepoutMargin);
        if (spec.hasCustomPadding)
            block->setContentPadding(spec.contentPadding);

        Canvas::Api::CanvasBlockStyle style;
        const bool hasStyle = m_styleHost && !spec.styleKey.trimmed().isEmpty()
                              && m_styleHost->hasBlockStyle(spec.styleKey);
        if (hasStyle)
            style = m_styleHost->blockStyle(spec.styleKey);

        const bool hasSpecColors = spec.hasCustomColors;
        const bool hasStyleColors = hasStyle && style.hasColors();
        if (hasSpecColors) {
            block->setCustomColors(spec.outlineColor, spec.fillColor, spec.labelColor);
        } else if (hasStyleColors) {
            const QColor outline = style.outlineColor.isValid() ? style.outlineColor : QColor(Constants::kBlockOutlineColor);
            const QColor fill = style.fillColor.isValid() ? style.fillColor : QColor(Constants::kBlockFillColor);
            const QColor label = style.labelColor.isValid() ? style.labelColor : QColor(Constants::kBlockTextColor);
            block->setCustomColors(outline, fill, label);
        } else {
            block->clearCustomColors();
        }

        double radius = -1.0;
        if (spec.cornerRadius >= 0.0)
            radius = spec.cornerRadius;
        else if (hasStyle && style.cornerRadius >= 0.0)
            radius = style.cornerRadius;
        if (radius >= 0.0)
            block->setCornerRadius(radius);
        else
            block->setCornerRadius(-1.0);

        CanvasBlockHandleImpl* handle = m_handles.value(spec.id, nullptr);
        if (!handle)
            handle = new CanvasBlockHandleImpl(spec.id, m_document, block, this);
        else
            handle->setBlock(block);

        nextHandles.insert(spec.id, handle);
    }

    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        if (!nextHandles.contains(it.key())) {
            if (it.value())
                it.value()->deleteLater();
        }
    }

    m_handles = std::move(nextHandles);
    emit blocksChanged();
}

void CanvasGridHostImpl::removeAllBlocks()
{
    if (!m_document)
        return;

    for (auto it = m_handles.begin(); it != m_handles.end(); ++it) {
        CanvasBlockHandleImpl* handle = it.value();
        if (!handle)
            continue;
        if (auto* block = handle->block())
            m_document->removeItem(block->id());
        handle->setBlock(nullptr);
    }
}

QSizeF CanvasGridHostImpl::resolveCellSize() const
{
    if (m_view) {
        const QSizeF viewportSize = m_view->size();
        return Utils::GridLayout::resolveCellSize(m_gridSpec, viewportSize, kFallbackCellSize);
    }
    return Utils::GridLayout::resolveCellSize(m_gridSpec, QSizeF(), kFallbackCellSize);
}

QRectF CanvasGridHostImpl::rectForBlock(const Canvas::Api::CanvasBlockSpec& spec,
                                        const QSizeF& cellSize) const
{
    return Utils::GridLayout::rectForGrid(m_gridSpec, spec.gridRect, cellSize);
}

} // namespace Canvas::Internal
