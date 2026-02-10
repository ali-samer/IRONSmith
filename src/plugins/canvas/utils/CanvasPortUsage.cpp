#include "canvas/utils/CanvasPortUsage.hpp"

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"

namespace Canvas::Support {

int countPortAttachments(const CanvasDocument& doc,
                         ObjectId itemId,
                         PortId portId,
                         ObjectId excludeWireId)
{
    int count = 0;
    for (const auto& it : doc.items()) {
        const auto* wire = dynamic_cast<const CanvasWire*>(it.get());
        if (!wire)
            continue;
        if (excludeWireId && wire->id() == excludeWireId)
            continue;
        const auto& a = wire->a();
        const auto& b = wire->b();
        if (a.attached && a.attached->itemId == itemId && a.attached->portId == portId)
            ++count;
        if (b.attached && b.attached->itemId == itemId && b.attached->portId == portId)
            ++count;
    }
    return count;
}

bool isPortAvailable(const CanvasDocument& doc,
                     ObjectId itemId,
                     PortId portId,
                     ObjectId excludeWireId)
{
    return countPortAttachments(doc, itemId, portId, excludeWireId) == 0;
}

} // namespace Canvas::Support
