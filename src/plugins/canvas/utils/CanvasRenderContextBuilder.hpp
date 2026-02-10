#pragma once

#include "canvas/CanvasRenderContext.hpp"

#include <QtCore/QRectF>

namespace Canvas {
class CanvasDocument;
class CanvasView;
}

namespace Canvas::Support {

struct RenderContextSelection final {
    CanvasRenderContext::IsSelectedFn isSelected = nullptr;
    void* user = nullptr;
};

struct RenderContextPortState final {
    bool hasHoveredPort = false;
    ObjectId hoveredPortItem{};
    PortId hoveredPortId{};

    bool hasSelectedPort = false;
    ObjectId selectedPortItem{};
    PortId selectedPortId{};
};

CanvasRenderContext buildRenderContext(const CanvasDocument* doc,
                                       const QRectF& visibleSceneRect,
                                       double zoom,
                                       const RenderContextSelection& selection = RenderContextSelection{},
                                       const RenderContextPortState& ports = RenderContextPortState{});

QRectF computeVisibleSceneRect(const CanvasView& view);

} // namespace Canvas::Support
