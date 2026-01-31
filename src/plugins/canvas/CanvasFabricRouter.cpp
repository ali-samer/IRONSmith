#include "canvas/CanvasFabricRouter.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <vector>

namespace Canvas {

namespace {

QVector<QPointF> simplifyPolyline(QVector<QPointF> pts)
{
    if (pts.size() < 3)
        return pts;

    QVector<QPointF> out;
    out.reserve(pts.size());
    out.push_back(pts.front());

    for (int i = 1; i + 1 < pts.size(); ++i) {
        const QPointF a = out.back();
        const QPointF b = pts[i];
        const QPointF c = pts[i + 1];

        const bool collinear = (std::abs(a.x() - b.x()) < 1e-6 && std::abs(b.x() - c.x()) < 1e-6)
                            || (std::abs(a.y() - b.y()) < 1e-6 && std::abs(b.y() - c.y()) < 1e-6);
        if (!collinear)
            out.push_back(b);
    }

    out.push_back(pts.back());
    return out;
}

QVector<QRectF> expandObstacles(const QVector<QRectF>& obstacles, double clearance)
{
    QVector<QRectF> out;
    out.reserve(obstacles.size());
    for (const auto& r : obstacles)
        out.push_back(r.adjusted(-clearance, -clearance, clearance, clearance));
    return out;
}

bool pointInsideObstacle(const QPointF& p, const QRectF& r)
{
    const double eps = 0.25;
    return (p.x() > r.left() + eps && p.x() < r.right() - eps
         && p.y() > r.top() + eps && p.y() < r.bottom() - eps);
}

bool segmentIntersectsObstacle(const QPointF& a, const QPointF& b, const QRectF& r)
{
    const double eps = 0.25;
    const QRectF interior(r.left() + eps,
                         r.top() + eps,
                         r.width() - 2.0 * eps,
                         r.height() - 2.0 * eps);
    if (interior.isEmpty())
        return false;

    if (std::abs(a.y() - b.y()) < 1e-6) {
        const double y = a.y();
        if (!(y > interior.top() && y < interior.bottom()))
            return false;
        const double x1 = std::min(a.x(), b.x());
        const double x2 = std::max(a.x(), b.x());
        return (x2 > interior.left() && x1 < interior.right());
    }

    if (std::abs(a.x() - b.x()) < 1e-6) {
        const double x = a.x();
        if (!(x > interior.left() && x < interior.right()))
            return false;
        const double y1 = std::min(a.y(), b.y());
        const double y2 = std::max(a.y(), b.y());
        return (y2 > interior.top() && y1 < interior.bottom());
    }

    return false;
}

bool segmentClear(const QPointF& a, const QPointF& b, const QVector<QRectF>& obstacles)
{
    for (const auto& r : obstacles) {
        if (segmentIntersectsObstacle(a, b, r))
            return false;
    }
    return true;
}

int indexOf(const QVector<double>& v, double x)
{
    const auto it = std::lower_bound(v.begin(), v.end(), x);
    if (it == v.end())
        return -1;
    if (std::abs(*it - x) > 1e-6)
        return -1;
    return int(it - v.begin());
}

struct Lattice
{
    QVector<double> xs;
    QVector<double> ys;
    int nx = 0;
    int ny = 0;
    QVector<int> nodeId;
    QVector<QPointF> nodes;

    int at(int ix, int iy) const
    {
        return nodeId[ix * ny + iy];
    }
};

Lattice buildLattice(const QVector<double>& xs,
                            const QVector<double>& ys,
                            const QVector<QRectF>& obstaclesExpanded)
{
    Lattice l;
    l.xs = xs;
    l.ys = ys;
    l.nx = xs.size();
    l.ny = ys.size();
    l.nodeId.fill(-1, l.nx * l.ny);
    l.nodes.reserve(l.nx * l.ny);

    auto isFree = [&](const QPointF& p) -> bool {
        for (const auto& r : obstaclesExpanded) {
            if (pointInsideObstacle(p, r))
                return false;
        }
        return true;
    };

    for (int ix = 0; ix < l.nx; ++ix) {
        for (int iy = 0; iy < l.ny; ++iy) {
            const QPointF p(xs[ix], ys[iy]);
            if (!isFree(p))
                continue;
            const int nid = l.nodes.size();
            l.nodeId[ix * l.ny + iy] = nid;
            l.nodes.push_back(p);
        }
    }

    return l;
}

QVector<QPointF> reconstructPath(int startId, int endId,
                                        const QVector<int>& prev,
                                        const QVector<QPointF>& nodes)
{
    QVector<QPointF> path;
    if (startId < 0 || endId < 0)
        return path;

    int cur = endId;
    while (cur != -1) {
        path.push_back(nodes[cur]);
        if (cur == startId)
            break;
        cur = prev[cur];
    }

    if (path.isEmpty() || path.back() != nodes[startId])
        return {};

    std::reverse(path.begin(), path.end());
    return simplifyPolyline(path);
}

} // namespace

FabricOverlay FabricRouter::buildOverlay(const QVector<double>& xs,
                                        const QVector<double>& ys,
                                        const QVector<QRectF>& obstacles,
                                        const Params& params)
{
    FabricOverlay ov;
    const QVector<QRectF> expanded = expandObstacles(obstacles, params.obstacleClearance);
    const Lattice l = buildLattice(xs, ys, expanded);

    ov.nodes = l.nodes;

    ov.edges.reserve(l.nodes.size());

    auto coordsOf = [&](int nid) -> std::pair<int, int> {
        const QPointF p = l.nodes[nid];
        const int ix = int(std::lower_bound(l.xs.begin(), l.xs.end(), p.x()) - l.xs.begin());
        const int iy = int(std::lower_bound(l.ys.begin(), l.ys.end(), p.y()) - l.ys.begin());
        return {ix, iy};
    };

    for (int nid = 0; nid < l.nodes.size(); ++nid) {
        const auto [ix, iy] = coordsOf(nid);
        const QPointF here = l.nodes[nid];

        for (int x = ix + 1; x < l.nx; ++x) {
            const int to = l.at(x, iy);
            if (to == -1)
                continue;
            const QPointF there = l.nodes[to];
            if (segmentClear(here, there, expanded))
                ov.edges.push_back(QLineF(here, there));
            break;
        }

        for (int y = iy + 1; y < l.ny; ++y) {
            const int to = l.at(ix, y);
            if (to == -1)
                continue;
            const QPointF there = l.nodes[to];
            if (segmentClear(here, there, expanded))
                ov.edges.push_back(QLineF(here, there));
            break;
        }
    }

    return ov;
}

FabricOverlay FabricRouter::buildOverlay(const QVector<double> &xs,
                                         const QVector<double> &ys,
                                         const QVector<QRectF> &obstacles) {
    return buildOverlay(xs, ys, obstacles, Params{});
}

QVector<QPointF> FabricRouter::route(const QPointF& start,
                                     const QPointF& end,
                                     const QVector<double>& xs,
                                     const QVector<double>& ys,
                                     const QVector<QRectF>& obstacles,
                                     const Params& params)
{
    const int sx = indexOf(xs, start.x());
    const int sy = indexOf(ys, start.y());
    const int ex = indexOf(xs, end.x());
    const int ey = indexOf(ys, end.y());

    if (sx == -1 || sy == -1 || ex == -1 || ey == -1)
        return { start, end };

    const QVector<QRectF> expanded = expandObstacles(obstacles, params.obstacleClearance);
    const Lattice l = buildLattice(xs, ys, expanded);

    const int startId = l.at(sx, sy);
    const int endId = l.at(ex, ey);
    if (startId == -1 || endId == -1)
        return { start, end };
    if (startId == endId)
        return { start };

    auto manhattan = [&](const QPointF& a, const QPointF& b) -> double {
        return std::abs(a.x() - b.x()) + std::abs(a.y() - b.y());
    };

    struct State
    {
        double f = 0.0;
        double g = 0.0;
        int id = -1;
    };

    struct Cmp
    {
        bool operator()(const State& a, const State& b) const
        {
            if (a.f != b.f)
                return a.f > b.f;
            if (a.g != b.g)
                return a.g > b.g;
            return a.id > b.id;
        }
    };

    std::priority_queue<State, std::vector<State>, Cmp> pq;

    QVector<double> gScore(l.nodes.size(), std::numeric_limits<double>::infinity());
    QVector<int> prev(l.nodes.size(), -1);

    gScore[startId] = 0.0;
    pq.push(State{manhattan(l.nodes[startId], l.nodes[endId]), 0.0, startId});

    auto coordsOf = [&](int nid) -> std::pair<int, int> {
        const QPointF p = l.nodes[nid];
        const int ix = int(std::lower_bound(l.xs.begin(), l.xs.end(), p.x()) - l.xs.begin());
        const int iy = int(std::lower_bound(l.ys.begin(), l.ys.end(), p.y()) - l.ys.begin());
        return {ix, iy};
    };

    while (!pq.empty()) {
        const State s = pq.top();
        pq.pop();

        if (s.g != gScore[s.id])
            continue;

        if (s.id == endId)
            break;

        const auto [ix, iy] = coordsOf(s.id);
        const QPointF here = l.nodes[s.id];

        auto relax = [&](int to) {
            const QPointF there = l.nodes[to];
            if (!segmentClear(here, there, expanded))
                return;

            const double w = manhattan(here, there);
            const double ng = s.g + w;
            if (ng < gScore[to]) {
                gScore[to] = ng;
                prev[to] = s.id;
                const double nf = ng + manhattan(there, l.nodes[endId]);
                pq.push(State{nf, ng, to});
            }
        };

        for (int x = ix + 1; x < l.nx; ++x) {
            const int to = l.at(x, iy);
            if (to == -1)
                continue;
            relax(to);
            break;
        }
        for (int x = ix - 1; x >= 0; --x) {
            const int to = l.at(x, iy);
            if (to == -1)
                continue;
            relax(to);
            break;
        }
        for (int y = iy + 1; y < l.ny; ++y) {
            const int to = l.at(ix, y);
            if (to == -1)
                continue;
            relax(to);
            break;
        }
        for (int y = iy - 1; y >= 0; --y) {
            const int to = l.at(ix, y);
            if (to == -1)
                continue;
            relax(to);
            break;
        }
    }

    if (prev[endId] == -1)
        return { start, end };

    QVector<QPointF> path = reconstructPath(startId, endId, prev, l.nodes);
    if (path.isEmpty())
        return { start, end };

    return path;
}

QVector<QPointF> FabricRouter::route(const QPointF &start,
                                     const QPointF &end,
                                     const QVector<double> &xs,
                                     const QVector<double> &ys,
                                     const QVector<QRectF> &obstacles) {
    return route(start, end, xs, ys, obstacles, Params{});
}

} // namespace Canvas
