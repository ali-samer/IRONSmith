#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QRectF>

namespace Canvas {

struct CANVAS_EXPORT CanvasRenderContext final {
    double zoom = 1.0;
    QRectF visibleSceneRect;

    using IsSelectedFn = bool (*)(void*, ObjectId);
    IsSelectedFn isSelected = nullptr;
    void* isSelectedUser = nullptr;

    bool selected(ObjectId id) const {
        return isSelected ? isSelected(isSelectedUser, id) : false;
    }

    using ComputePortTerminalFn = bool (*)(void*, ObjectId, PortId, QPointF& outAnchor, QPointF& outBorder, QPointF& outFabric);
    ComputePortTerminalFn computePortTerminal = nullptr;
    void* computePortTerminalUser = nullptr;

    bool portTerminal(ObjectId itemId, PortId portId, QPointF& outAnchor, QPointF& outBorder, QPointF& outFabric) const {
        return computePortTerminal ? computePortTerminal(computePortTerminalUser, itemId, portId, outAnchor, outBorder, outFabric) : false;
    }

    using IsFabricBlockedFn = bool (*)(const FabricCoord& coord, void* user);
    IsFabricBlockedFn isFabricBlocked = nullptr;
    void* isFabricBlockedUser = nullptr;
    double fabricStep = 16.0;

    bool fabricBlocked(const FabricCoord& coord) const {
        return isFabricBlocked ? isFabricBlocked(coord, isFabricBlockedUser) : false;
    }

    bool hasHoveredPort = false;
    ObjectId hoveredPortItem{};
    PortId hoveredPortId{};

    bool portHovered(ObjectId itemId, PortId portId) const {
        return hasHoveredPort && hoveredPortItem == itemId && hoveredPortId == portId;
    }

    bool hasSelectedPort = false;
    ObjectId selectedPortItem{};
    PortId selectedPortId{};

    using IsPortSelectedFn = bool (*)(void*, ObjectId, PortId);
    IsPortSelectedFn isPortSelected = nullptr;
    void* isPortSelectedUser = nullptr;

    bool portSelected(ObjectId itemId, PortId portId) const {
        if (isPortSelected)
            return isPortSelected(isPortSelectedUser, itemId, portId);
        return hasSelectedPort && selectedPortItem == itemId && selectedPortId == portId;
    }
};

} // namespace Canvas
