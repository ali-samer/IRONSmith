#include "canvas/internal/CanvasBlockHandleImpl.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasBlockContent.hpp"
#include "canvas/CanvasDocument.hpp"

namespace Canvas::Internal {

CanvasBlockHandleImpl::CanvasBlockHandleImpl(QString id,
                                             CanvasDocument* document,
                                             CanvasBlock* block,
                                             QObject* parent)
    : Canvas::Api::ICanvasBlockHandle(parent)
    , m_id(std::move(id))
    , m_document(document)
{
    setBlock(block);
}

QString CanvasBlockHandleImpl::id() const
{
    return m_id;
}

void CanvasBlockHandleImpl::setLabel(const QString& label)
{
    auto* blk = block();
    if (!blk)
        return;
    if (blk->label() == label)
        return;
    blk->setLabel(label);
    if (m_document)
        m_document->notifyChanged();
    emit labelChanged(label);
}

void CanvasBlockHandleImpl::setMovable(bool movable)
{
    auto* blk = block();
    if (!blk)
        return;
    if (blk->isMovable() == movable)
        return;
    blk->setMovable(movable);
    if (m_document)
        m_document->notifyChanged();
    emit movableChanged(movable);
}

void CanvasBlockHandleImpl::setShowPorts(bool show)
{
    auto* blk = block();
    if (!blk)
        return;
    if (blk->showPorts() == show)
        return;
    blk->setShowPorts(show);
    if (m_document)
        m_document->notifyChanged();
    emit showPortsChanged(show);
}

void CanvasBlockHandleImpl::setKeepoutMargin(double marginScene)
{
    auto* blk = block();
    if (!blk)
        return;
    if (qFuzzyCompare(blk->keepoutMargin(), marginScene))
        return;
    blk->setKeepoutMargin(marginScene);
    if (m_document)
        m_document->notifyChanged();
    emit keepoutMarginChanged(marginScene);
}

void CanvasBlockHandleImpl::setContentPadding(const QMarginsF& padding)
{
    auto* blk = block();
    if (!blk)
        return;
    if (blk->contentPadding() == padding)
        return;
    blk->setContentPadding(padding);
    if (m_document)
        m_document->notifyChanged();
    emit contentPaddingChanged(padding);
}

void CanvasBlockHandleImpl::setContent(std::unique_ptr<Canvas::BlockContent> content)
{
    auto* blk = block();
    if (!blk)
        return;
    blk->setContent(std::move(content));
    if (m_document)
        m_document->notifyChanged();
    emit contentChanged();
}

Canvas::BlockContent* CanvasBlockHandleImpl::content() const
{
    if (auto* blk = block())
        return blk->content();
    return nullptr;
}

CanvasBlock* CanvasBlockHandleImpl::block() const
{
    if (!m_document || m_blockId.isNull())
        return nullptr;

    auto* item = m_document->findItem(m_blockId);
    return dynamic_cast<CanvasBlock*>(item);
}

void CanvasBlockHandleImpl::setBlock(CanvasBlock* block)
{
    m_blockId = block ? block->id() : ObjectId{};
}

} // namespace Canvas::Internal
