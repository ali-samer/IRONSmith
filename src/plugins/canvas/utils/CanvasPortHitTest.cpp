#include "canvas/utils/CanvasPortHitTest.hpp"

#include "canvas/CanvasConstants.hpp"

#include <algorithm>
#include <cmath>

namespace Canvas::Utils {

namespace {

double distanceToSegment(const QPointF& p, const QPointF& a, const QPointF& b)
{
    const QPointF ab = b - a;
    const double len2 = ab.x() * ab.x() + ab.y() * ab.y();
    if (len2 <= 1e-9)
        return std::hypot(p.x() - a.x(), p.y() - a.y());

    const QPointF ap = p - a;
    double t = (ap.x() * ab.x() + ap.y() * ab.y()) / len2;
    t = std::clamp(t, 0.0, 1.0);
    const QPointF proj = a + ab * t;
    return std::hypot(p.x() - proj.x(), p.y() - proj.y());
}

QPointF sideDir(PortSide side)
{
    switch (side) {
        case PortSide::Left: return QPointF(-1.0, 0.0);
        case PortSide::Right: return QPointF(1.0, 0.0);
        case PortSide::Top: return QPointF(0.0, -1.0);
        case PortSide::Bottom: return QPointF(0.0, 1.0);
    }
    return QPointF(0.0, 0.0);
}

} // namespace

bool hitTestPortGeometry(const QPointF& anchorScene,
                         PortSide side,
                         const QPointF& scenePos,
                         double radiusScene)
{
    const double stubLen = Constants::kPortHitStubLengthPx;
    const double half = Constants::kPortHitBoxHalfPx;
    const double hitRadius = std::max(radiusScene, half);

    const QPointF dir = sideDir(side);
    const QPointF stubEnd = anchorScene + dir * stubLen;

    const double segDist = distanceToSegment(scenePos, anchorScene, stubEnd);
    if (segDist <= hitRadius)
        return true;

    const QRectF box(anchorScene.x() - half, anchorScene.y() - half, half * 2.0, half * 2.0);
    return box.contains(scenePos);
}

} // namespace Canvas::Utils
