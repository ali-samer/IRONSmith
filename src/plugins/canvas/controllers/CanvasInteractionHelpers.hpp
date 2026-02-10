#pragma once

#include "canvas/CanvasInteractionTypes.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasWire.hpp"

#include <QtCore/QPointF>

#include <optional>
#include <vector>

namespace Canvas {
class CanvasBlock;
class CanvasDocument;
class CanvasView;
class CanvasWire;
struct PortRef;
}

namespace Canvas::Controllers::Detail {

struct WireEndpointHit final {
    bool isA = true;
    CanvasWire::Endpoint endpoint;
};

struct EndpointCandidate final {
    CanvasWire* wire = nullptr;
    WireEndpointHit hit;
};

CanvasRenderContext buildRenderContext(const CanvasDocument* doc,
                                       const CanvasView* view);

std::optional<EndpointCandidate> pickEndpointCandidate(CanvasDocument* doc,
                                                       const CanvasView* view,
                                                       const QPointF& scenePos,
                                                       double tol);

std::optional<WireEndpointHit> pickWireEndpoint(CanvasWire* wire,
                                                const CanvasRenderContext& ctx,
                                                const QPointF& scenePos,
                                                double tol);

std::optional<EdgeCandidate> edgeCandidateAt(CanvasDocument* doc,
                                             CanvasView* view,
                                             const QPointF& scenePos);

std::optional<PortRef> ensureEdgePort(CanvasDocument* doc,
                                      const EdgeCandidate& candidate);

bool findPortIndex(const CanvasBlock& block, PortId portId, size_t& outIndex);

int pickWireSegment(const std::vector<QPointF>& path,
                    const QPointF& scenePos,
                    double tol,
                    bool& outHorizontal);

int adjustSegmentCoord(const CanvasDocument* doc,
                       bool horizontal,
                       int desired,
                       int spanMin,
                       int spanMax);

} // namespace Canvas::Controllers::Detail
