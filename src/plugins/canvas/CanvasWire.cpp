#include "canvas/CanvasWire.hpp"

#include "canvas/CanvasStyle.hpp"
#include "canvas/internal/CanvasWireRouting.hpp"
#include "canvas/utils/CanvasGeometry.hpp"

#include <QtCore/QLineF>
#include <QtGui/QPainter>

#include <utility>
#include <vector>

namespace Canvas {

namespace {

constexpr double kHitTestTolerance = 6.0;
constexpr int kEscapeMaxSteps = 8;

int signum(double v)
{
    return (v > 0.0) ? 1 : (v < 0.0) ? -1 : 0;
}

double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
    if (len2 <= 1e-6)
        return QLineF(p, a).length();

    const QPointF ap = p - a;
    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / len2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    const QPointF proj(a.x() + t * ab.x(), a.y() + t * ab.y());
    return QLineF(p, proj).length();
}

static QPointF endpointScene(const CanvasWire::Endpoint& e, const CanvasRenderContext& ctx,
                             QPointF* outAnchor, QPointF* outBorder, QPointF* outFabric)
{
    if (e.attached.has_value()) {
        const auto ref = *e.attached;
        QPointF a, b, f;
        if (ctx.portTerminal(ref.itemId, ref.portId, a, b, f)) {
            if (outAnchor) *outAnchor = a;
            if (outBorder) *outBorder = b;
            if (outFabric) *outFabric = f;
            return f;
        }
    }
    if (outAnchor) *outAnchor = e.freeScene;
    if (outBorder) *outBorder = e.freeScene;
    if (outFabric) *outFabric = e.freeScene;
    return e.freeScene;
}

struct EndpointPositions final {
    QPointF aAnchor;
    QPointF aBorder;
    QPointF aFabric;
    QPointF bAnchor;
    QPointF bBorder;
    QPointF bFabric;
};

EndpointPositions resolveEndpoints(const CanvasWire& wire, const CanvasRenderContext& ctx)
{
    EndpointPositions out;
    endpointScene(wire.a(), ctx, &out.aAnchor, &out.aBorder, &out.aFabric);
    endpointScene(wire.b(), ctx, &out.bAnchor, &out.bBorder, &out.bFabric);
    return out;
}

struct EscapeInfo final {
    FabricCoord coord{};
    bool hasEscape = false;
};

EscapeInfo computeEscape(const QPointF& borderScene,
                         const QPointF& fabricScene,
                         const FabricCoord& start,
                         const CanvasRenderContext& ctx)
{
    const int dirX = signum(fabricScene.x() - borderScene.x());
    const int dirY = signum(fabricScene.y() - borderScene.y());
    if (dirX == 0 && dirY == 0)
        return {start, false};

    FabricCoord cur = start;
    for (int step = 0; step < kEscapeMaxSteps; ++step) {
        FabricCoord next{cur.x + dirX, cur.y + dirY};
        if (!ctx.fabricBlocked(next))
            return {next, true};
        cur = next;
    }
    return {start, false};
}

void appendCoord(std::vector<FabricCoord>& path, const FabricCoord& coord)
{
    if (!path.empty() && path.back().x == coord.x && path.back().y == coord.y)
        return;
    path.push_back(coord);
}

} // namespace


void CanvasWire::draw(QPainter& p, const CanvasRenderContext& ctx) const
{
    QPointF aAnchor, aBorder, aFabric;
    QPointF bAnchor, bBorder, bFabric;
    endpointScene(m_a, ctx, &aAnchor, &aBorder, &aFabric);
    endpointScene(m_b, ctx, &bAnchor, &bBorder, &bFabric);

    const std::vector<QPointF> route = resolvedPathScene(ctx);

    CanvasStyle::drawWirePath(p, aAnchor, aBorder, aFabric, bFabric, bBorder, bAnchor, route,
                              ctx.zoom, ctx.selected(id()));
}

QRectF CanvasWire::boundsScene() const
{
    QRectF r(m_a.freeScene, m_b.freeScene);
    return r.normalized().adjusted(-8.0, -8.0, 8.0, 8.0);
}

std::unique_ptr<CanvasItem> CanvasWire::clone() const
{
    auto w = std::make_unique<CanvasWire>(m_a, m_b);
    w->setId(id());
    w->m_routeOverride = m_routeOverride;
    return w;
}

bool CanvasWire::hitTest(const QPointF& scenePos) const
{
    return distanceToSegment(scenePos, m_a.freeScene, m_b.freeScene) <= kHitTestTolerance;
}

bool CanvasWire::hitTest(const QPointF& scenePos, const CanvasRenderContext& ctx) const
{
    const std::vector<QPointF> route = resolvedPathScene(ctx);

    if (route.size() >= 2) {
        for (size_t i = 1; i < route.size(); ++i) {
            if (distanceToSegment(scenePos, route[i - 1], route[i]) <= kHitTestTolerance)
                return true;
        }
        return false;
    }
    if (route.size() == 1)
        return QLineF(scenePos, route.front()).length() <= kHitTestTolerance;
    return false;
}

void CanvasWire::setRouteOverride(std::vector<FabricCoord> path)
{
    m_routeOverride = std::move(path);
    m_overrideStale = false;
    if (!m_routeOverride.empty()) {
        m_overrideStart = m_routeOverride.front();
        m_overrideEnd = m_routeOverride.back();
    }
}

void CanvasWire::clearRouteOverride()
{
    m_routeOverride.clear();
    m_overrideStale = false;
}

bool CanvasWire::attachesTo(ObjectId itemId) const
{
    return (m_a.attached.has_value() && m_a.attached->itemId == itemId) ||
           (m_b.attached.has_value() && m_b.attached->itemId == itemId);
}

std::vector<QPointF> CanvasWire::resolvedPathScene(const CanvasRenderContext& ctx) const
{
    const EndpointPositions endpoints = resolveEndpoints(*this, ctx);
    const Internal::WireRouter router(ctx);
    const double step = ctx.fabricStep;

    if (step <= 0.0)
        return router.routeFabricPath(endpoints.aFabric, endpoints.bFabric);

    const FabricCoord startCoord = Utils::toFabricCoord(endpoints.aFabric, step);
    const FabricCoord endCoord = Utils::toFabricCoord(endpoints.bFabric, step);
    const EscapeInfo escapeA = computeEscape(endpoints.aBorder, endpoints.aFabric, startCoord, ctx);
    const EscapeInfo escapeB = computeEscape(endpoints.bBorder, endpoints.bFabric, endCoord, ctx);

    const FabricCoord routeStart = escapeA.hasEscape ? escapeA.coord : startCoord;
    const FabricCoord routeEnd = escapeB.hasEscape ? escapeB.coord : endCoord;

    bool useOverride = !m_routeOverride.empty() && !m_overrideStale;
    if (useOverride) {
        if (startCoord.x != m_overrideStart.x || startCoord.y != m_overrideStart.y ||
            endCoord.x != m_overrideEnd.x || endCoord.y != m_overrideEnd.y) {
            m_overrideStale = true;
            useOverride = false;
        }
    }

    std::vector<FabricCoord> core;
    if (!useOverride) {
        core = router.routeCoords(routeStart, routeEnd);
    } else {
        std::vector<FabricCoord> waypoints = m_routeOverride;
        if (!waypoints.empty()) {
            waypoints.front() = routeStart;
            waypoints.back() = routeEnd;
        }
        core = router.routeCoordsViaWaypoints(waypoints);
    }

    std::vector<FabricCoord> full;
    appendCoord(full, startCoord);
    if (escapeA.hasEscape)
        appendCoord(full, routeStart);
    for (const auto& coord : core)
        appendCoord(full, coord);
    if (escapeB.hasEscape)
        appendCoord(full, endCoord);

    return Internal::simplifyCoordsToScene(full, step, endpoints.aFabric, endpoints.bFabric);
}

std::vector<FabricCoord> CanvasWire::resolvedPathCoords(const CanvasRenderContext& ctx) const
{
    const double step = ctx.fabricStep;
    if (step <= 0.0)
        return {};

    const std::vector<QPointF> pathScene = resolvedPathScene(ctx);
    std::vector<FabricCoord> out;
    out.reserve(pathScene.size());
    for (const auto& pt : pathScene)
        out.push_back(Utils::toFabricCoord(pt, step));
    return out;
}

} // namespace Canvas
