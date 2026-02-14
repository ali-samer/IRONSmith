#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointF>

namespace Canvas {
class CanvasDocument;
}

namespace Canvas::Services {

class CANVAS_EXPORT CanvasGeometryService final
{
public:
    static bool isFabricPointBlocked(const CanvasDocument& doc, const FabricCoord& coord);

    static bool computePortTerminal(const CanvasDocument& doc,
                                    ObjectId itemId,
                                    PortId portId,
                                    QPointF& outAnchorScene,
                                    QPointF& outBorderScene,
                                    QPointF& outFabricScene);

    static bool computePortTerminalThunk(void* user,
                                         ObjectId itemId,
                                         PortId portId,
                                         QPointF& outAnchorScene,
                                         QPointF& outBorderScene,
                                         QPointF& outFabricScene);

    static bool isFabricPointBlockedThunk(const FabricCoord& coord, void* user);
};

} // namespace Canvas::Services
