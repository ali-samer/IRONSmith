#include "canvas/CanvasLinkRouteEditor.hpp"
#include "canvas/CanvasFabricRouter.hpp"

#include <QtCore/QLineF>

#include <algorithm>
#include <cmath>

namespace Canvas {
namespace {

constexpr double kEps = 1e-6;

static bool nearEq(double a, double b) { return std::abs(a - b) < kEps; }

static bool isHorizontal(const QPointF& a, const QPointF& b)
{
    return nearEq(a.y(), b.y()) && !nearEq(a.x(), b.x());
}

static bool isVertical(const QPointF& a, const QPointF& b)
{
    return nearEq(a.x(), b.x()) && !nearEq(a.y(), b.y());
}

static double nearestOnAxis(const QVector<double>& axis, double v)
{
    if (axis.isEmpty())
        return v;

    auto it = std::lower_bound(axis.begin(), axis.end(), v);
    if (it == axis.begin())
        return *it;
    if (it == axis.end())
        return axis.back();

    const double hi = *it;
    const double lo = *(it - 1);
    return (std::abs(hi - v) < std::abs(v - lo)) ? hi : lo;
}

static void pushNoDup(QVector<QPointF>& out, const QPointF& p)
{
    if (out.isEmpty() || !nearEq(out.back().x(), p.x()) || !nearEq(out.back().y(), p.y()))
        out.push_back(p);
}

static QVector<QPointF> simplifyPolyline(const QVector<QPointF>& pts)
{
    if (pts.size() < 3)
        return pts;

    QVector<QPointF> out;
    out.reserve(pts.size());
    out.push_back(pts.front());
    auto strictlyBetween = [](double a, double c, double b) {
        const double lo = std::min(a, c);
        const double hi = std::max(a, c);
        return (b > lo + kEps) && (b < hi - kEps);
    };

    for (int i = 1; i + 1 < pts.size(); ++i) {
        const QPointF a = out.back();
        const QPointF b = pts[i];
        const QPointF c = pts[i + 1];

        if (nearEq(a.x(), b.x()) && nearEq(b.x(), c.x())) {
            if (!strictlyBetween(a.y(), c.y(), b.y()))
                out.push_back(b);
            continue;
        }

        if (nearEq(a.y(), b.y()) && nearEq(b.y(), c.y())) {
            if (!strictlyBetween(a.x(), c.x(), b.x()))
                out.push_back(b);
            continue;
        }

        out.push_back(b);
    }
    out.push_back(pts.back());
    return out;
}

static bool segmentIntersectsInterior(const QPointF& a, const QPointF& b, const QRectF& r)
{
    const double eps = 0.25;
    const QRectF interior(r.left() + eps,
                         r.top() + eps,
                         r.width() - 2.0 * eps,
                         r.height() - 2.0 * eps);
    if (interior.isEmpty())
        return false;

    if (isHorizontal(a, b)) {
        const double y = a.y();
        if (!(y > interior.top() && y < interior.bottom()))
            return false;
        const double x1 = std::min(a.x(), b.x());
        const double x2 = std::max(a.x(), b.x());
        return (x2 > interior.left() && x1 < interior.right());
    }

    if (isVertical(a, b)) {
        const double x = a.x();
        if (!(x > interior.left() && x < interior.right()))
            return false;
        const double y1 = std::min(a.y(), b.y());
        const double y2 = std::max(a.y(), b.y());
        return (y2 > interior.top() && y1 < interior.bottom());
    }

    return true; // diagonals are always invalid
}

static bool validateAgainstObstacles(const QVector<QPointF>& pts,
                                    const QVector<QRectF>& obstacles,
                                    double clearance)
{
    if (pts.size() < 2)
        return false;

    for (int i = 0; i + 1 < pts.size(); ++i) {
        const QPointF a = pts[i];
        const QPointF b = pts[i + 1];
        if (!isHorizontal(a, b) && !isVertical(a, b))
            return false;

        for (const QRectF& o : obstacles) {
            const QRectF expanded = o.adjusted(-clearance, -clearance, clearance, clearance);
            if (segmentIntersectsInterior(a, b, expanded))
                return false;
        }
    }
    return true;
}

} // namespace

LinkRouteEditor::Result LinkRouteEditor::shiftSegmentToNearestLane(const QVector<QPointF>& worldPolyline,
                                                                   int segIndex,
                                                                   const QPointF& mouseWorld,
                                                                   const QVector<double>& xs,
                                                                   const QVector<double>& ys,
                                                                   const QVector<QRectF>& obstacles,
                                                                   double clearance)
{
    Result r;
    r.worldPoints = worldPolyline;
    r.ok = false;

    if (worldPolyline.size() < 2)
        return r;
    if (segIndex < 0 || segIndex >= worldPolyline.size() - 1)
        return r;

    const QPointF p0 = worldPolyline[segIndex];
    const QPointF p1 = worldPolyline[segIndex + 1];
    const bool horiz = isHorizontal(p0, p1);
    const bool vert = isVertical(p0, p1);
    if (!horiz && !vert)
        return r;

    int runStart = segIndex;
    int runEnd = segIndex + 1;
    if (horiz) {
        const double y = p0.y();
        while (runStart > 0 && isHorizontal(worldPolyline[runStart - 1], worldPolyline[runStart])
               && nearEq(worldPolyline[runStart].y(), y)) {
            --runStart;
        }
        while (runEnd < worldPolyline.size() - 1 && isHorizontal(worldPolyline[runEnd], worldPolyline[runEnd + 1])
               && nearEq(worldPolyline[runEnd].y(), y)) {
            ++runEnd;
        }
    } else {
        const double x = p0.x();
        while (runStart > 0 && isVertical(worldPolyline[runStart - 1], worldPolyline[runStart])
               && nearEq(worldPolyline[runStart].x(), x)) {
            --runStart;
        }
        while (runEnd < worldPolyline.size() - 1 && isVertical(worldPolyline[runEnd], worldPolyline[runEnd + 1])
               && nearEq(worldPolyline[runEnd].x(), x)) {
            ++runEnd;
        }
    }

    r.runStartPointIndex = runStart;
    r.runEndPointIndex = runEnd;
    r.horizontalRun = horiz;

    const QPointF A = worldPolyline[runStart];
    const QPointF B = worldPolyline[runEnd];

    if (horiz) {
        const double targetY = nearestOnAxis(ys, mouseWorld.y());
        r.snappedCoord = targetY;

        if (nearEq(targetY, p0.y()))
            return r;

        const QPointF A2(A.x(), targetY);
        const QPointF B2(B.x(), targetY);

        QVector<QPointF> runSeg;
        runSeg.reserve(2);
        runSeg.push_back(A2);
        runSeg.push_back(B2);
        if (!validateAgainstObstacles(runSeg, obstacles, clearance))
            return r;

        FabricRouter::Params params;
        params.obstacleClearance = clearance;

        QVector<QPointF> out;
        out.reserve(worldPolyline.size() + 16);

        for (int i = 0; i < runStart; ++i)
            pushNoDup(out, worldPolyline[i]);

        const QVector<QPointF> aLeg = FabricRouter::route(A, A2, xs, ys, obstacles, params);
        for (const QPointF& p : aLeg)
            pushNoDup(out, p);

        pushNoDup(out, B2);

        const QVector<QPointF> bLeg = FabricRouter::route(B2, B, xs, ys, obstacles, params);
        for (int i = 1; i < bLeg.size(); ++i)
            pushNoDup(out, bLeg[i]);

        for (int i = runEnd + 1; i < worldPolyline.size(); ++i)
            pushNoDup(out, worldPolyline[i]);

        out = simplifyPolyline(out);
        r.worldPoints = out;
        r.ok = validateAgainstObstacles(out, obstacles, clearance);
        return r;
    }

    const double targetX = nearestOnAxis(xs, mouseWorld.x());
    r.snappedCoord = targetX;

    if (nearEq(targetX, p0.x()))
        return r;

    const QPointF A2(targetX, A.y());
    const QPointF B2(targetX, B.y());

    QVector<QPointF> runSeg;
    runSeg.reserve(2);
    runSeg.push_back(A2);
    runSeg.push_back(B2);
    if (!validateAgainstObstacles(runSeg, obstacles, clearance))
        return r;

    FabricRouter::Params params;
    params.obstacleClearance = clearance;

    QVector<QPointF> out;
    out.reserve(worldPolyline.size() + 16);

    for (int i = 0; i < runStart; ++i)
        pushNoDup(out, worldPolyline[i]);

    const QVector<QPointF> aLeg = FabricRouter::route(A, A2, xs, ys, obstacles, params);
    for (const QPointF& p : aLeg)
        pushNoDup(out, p);

    pushNoDup(out, B2);

    const QVector<QPointF> bLeg = FabricRouter::route(B2, B, xs, ys, obstacles, params);
    for (int i = 1; i < bLeg.size(); ++i)
        pushNoDup(out, bLeg[i]);

    for (int i = runEnd + 1; i < worldPolyline.size(); ++i)
        pushNoDup(out, worldPolyline[i]);

    out = simplifyPolyline(out);
    r.worldPoints = out;
    r.ok = validateAgainstObstacles(out, obstacles, clearance);
    return r;
}

} // namespace Canvas
