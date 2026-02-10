#include "canvas/services/CanvasHitTestService.hpp"

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasItem.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/utils/CanvasRenderContextBuilder.hpp"

namespace Canvas::Services {

CanvasItem* hitTestItem(const CanvasDocument& doc,
                        const QPointF& scenePos,
                        const CanvasRenderContext* ctx)
{
    CanvasRenderContext localCtx;
    const CanvasRenderContext* activeCtx = ctx;
    if (!activeCtx) {
        localCtx = Support::buildRenderContext(&doc, QRectF(), 1.0);
        activeCtx = &localCtx;
    }

    for (auto it = doc.items().rbegin(); it != doc.items().rend(); ++it) {
        CanvasItem* item = it->get();
        if (!item)
            continue;
        if (auto* wire = dynamic_cast<CanvasWire*>(item)) {
            if (wire->hitTest(scenePos, *activeCtx))
                return item;
            continue;
        }
        if (item->hitTest(scenePos))
            return item;
    }

    return nullptr;
}

CanvasBlock* hitTestBlock(const CanvasDocument& doc,
                          const QPointF& scenePos)
{
    for (auto it = doc.items().rbegin(); it != doc.items().rend(); ++it) {
        CanvasItem* item = it->get();
        if (!item)
            continue;
        if (auto* block = dynamic_cast<CanvasBlock*>(item)) {
            if (block->hitTest(scenePos))
                return block;
        }
    }

    return nullptr;
}

} // namespace Canvas::Services
