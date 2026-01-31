#include "canvas/CanvasSceneModel.hpp"

#include "canvas/CanvasFabricRouter.hpp"

#include <designmodel/DesignEntities.hpp>

#include <QtCore/QSet>

#include <limits>
#include <queue>
#include <vector>

#include <algorithm>
#include <optional>

namespace Canvas {

static quint64 keyFor(const DesignModel::TileCoord& c)
{
    return (static_cast<quint64>(static_cast<quint32>(c.row())) << 32)
         | static_cast<quint32>(c.col());
}

CanvasSceneModel::CanvasSceneModel() = default;

void CanvasSceneModel::setGridSpec(GridSpec spec)
{
    m_spec = spec;
}

QRectF CanvasSceneModel::worldRectToScreen(const QRectF& world, const CanvasViewport& vp) const
{
    const QPointF tl = vp.worldToScreen(world.topLeft());
    const double z = vp.zoomFactor();
    return QRectF(tl, QSizeF(world.width() * z, world.height() * z));
}

void CanvasSceneModel::rebuild(const DesignModel::DesignDocument& doc,
                              const CanvasViewport& vp,
                              const CanvasRenderOptions& opts)
{
    m_tiles.clear();
    m_hotspots.clear();
    m_blocks.clear();
    m_links.clear();
    m_annotations.clear();
    m_fabricNodes.clear();
    m_fabricEdges.clear();
    m_fabricXs.clear();
    m_fabricYs.clear();
    m_fabricObstacles.clear();
    m_tileWorldRects.clear();

    m_computeRectsScreen.clear();
    m_computeRectsWorld.clear();
    m_portCentersScreen.clear();
    m_portCentersWorld.clear();
    m_portSides.clear();
    m_portAnchors.clear();

    buildTiles(vp);
    buildBlocks(doc);
    buildHotspots(doc, vp);
    buildFabric(vp, opts);
    buildLinks(doc, vp);
    buildAnnotations(doc, opts);
}

void CanvasSceneModel::setLinkRoutePreview(DesignModel::LinkId id, QVector<QPointF> worldPolyline)
{
    if (id.isNull())
        return;
    if (worldPolyline.size() < 2) {
        m_linkRoutePreviewWorld.remove(id);
        return;
    }
    m_linkRoutePreviewWorld.insert(id, std::move(worldPolyline));
}

void CanvasSceneModel::clearLinkRoutePreview(DesignModel::LinkId id)
{
    m_linkRoutePreviewWorld.remove(id);
}

void CanvasSceneModel::clearAllLinkRoutePreviews()
{
    m_linkRoutePreviewWorld.clear();
}

void CanvasSceneModel::buildTiles(const CanvasViewport& vp)
{
    const double pitch = m_spec.tileSize + m_spec.tileGap;
    const double left = m_spec.margin;
    const double top = m_spec.margin;

    for (int col = 0; col < m_spec.aieCols; ++col) {
        for (int row = 0; row < m_spec.aieRows; ++row) {
            const int invRow = (m_spec.aieRows - 1) - row;
            const QRectF world(left + col * pitch,
                               top + invRow * pitch,
                               m_spec.tileSize,
                               m_spec.tileSize);
            TileVisual tv;
            tv.kind = DesignModel::TileKind::Aie;
            tv.coord = DesignModel::TileCoord(row, col);
            tv.rect = worldRectToScreen(world, vp);
            tv.label = QString("(%1,%2)").arg(col).arg(row);
            m_tiles.push_back(tv);
            m_computeRectsScreen.insert(keyFor(tv.coord), tv.rect);
            m_computeRectsWorld.insert(keyFor(tv.coord), world);
            m_tileWorldRects.push_back(world);
        }
    }

    const double memTopWorld = top + m_spec.aieRows * pitch + m_spec.bandGap;
    for (int col = 0; col < m_spec.aieCols; ++col) {
        for (int r = 0; r < m_spec.memRows; ++r) {
            const QRectF world(left + col * pitch,
                               memTopWorld + r * pitch,
                               m_spec.tileSize,
                               m_spec.tileSize);
            TileVisual tv;
            tv.kind = DesignModel::TileKind::Mem;
            tv.coord = DesignModel::TileCoord(0, col);
            tv.rect = worldRectToScreen(world, vp);
            tv.label = QString("MEM %1").arg(col);
            m_tiles.push_back(tv);
            m_tileWorldRects.push_back(world);
        }
    }

    const double shimTopWorld = memTopWorld + m_spec.memRows * pitch + m_spec.bandGap;
    for (int col = 0; col < m_spec.aieCols; ++col) {
        for (int r = 0; r < m_spec.shimRows; ++r) {
            const QRectF world(left + col * pitch,
                               shimTopWorld + r * pitch,
                               m_spec.tileSize,
                               m_spec.tileSize);
            TileVisual tv;
            tv.kind = DesignModel::TileKind::Shim;
            tv.coord = DesignModel::TileCoord(0, col);
            tv.rect = worldRectToScreen(world, vp);
            tv.label = QString("SHIM %1").arg(col);
            m_tiles.push_back(tv);
            m_tileWorldRects.push_back(world);
        }
    }
}

void CanvasSceneModel::buildBlocks(const DesignModel::DesignDocument& doc)
{
    for (const auto id : doc.blockIds()) {
        const auto* b = doc.tryBlock(id);
        if (!b)
            continue;
        const auto anchor = b->placement().anchor();
        const QRectF tr = computeTileRect(anchor);
        if (tr.isEmpty())
            continue;

        const QRectF inset = tr.adjusted(tr.width() * 0.15,
                                         tr.height() * 0.15,
                                         -tr.width() * 0.15,
                                         -tr.height() * 0.15);
        BlockVisual bv;
        bv.rect = inset;
        bv.id = id;
        switch (b->type()) {
        case DesignModel::BlockType::Compute: bv.text = "AIE"; break;
        case DesignModel::BlockType::Memory: bv.text = "MEM"; break;
        case DesignModel::BlockType::ShimInterface: bv.text = "SHIM"; break;
        case DesignModel::BlockType::Ddr: bv.text = "DDR"; break;
        default: bv.text = "BLOCK"; break;
        }
        m_blocks.push_back(bv);
    }
}

static QString sideToken(PortSide side)
{
    switch (side) {
    case PortSide::North: return QStringLiteral("N");
    case PortSide::East:  return QStringLiteral("E");
    case PortSide::South: return QStringLiteral("S");
    case PortSide::West:  return QStringLiteral("W");
    }
    return QString();
}

static bool matchesSide(const QString& name, PortSide side)
{
    const QString t = name.trimmed().toUpper();
    const QString tok = sideToken(side);
    if (t == tok)
        return true;
    return t.endsWith(QStringLiteral("_") + tok);
}

struct SidePorts
{
    DesignModel::PortId north;
    DesignModel::PortId east;
    DesignModel::PortId south;
    DesignModel::PortId west;

    DesignModel::PortId forSide(PortSide s) const
    {
        switch (s) {
        case PortSide::North: return north;
        case PortSide::East:  return east;
        case PortSide::South: return south;
        case PortSide::West:  return west;
        }
        return {};
    }

    void set(PortSide s, DesignModel::PortId id)
    {
        switch (s) {
        case PortSide::North: north = id; break;
        case PortSide::East:  east  = id; break;
        case PortSide::South: south = id; break;
        case PortSide::West:  west  = id; break;
        }
    }
};

static SidePorts assignPortsForSides(const DesignModel::DesignDocument& doc,
                                    DesignModel::BlockId bid)
{
    SidePorts out;

    const auto& ports = doc.index().portsForBlock(bid);
    if (ports.isEmpty())
        return out;

    for (const auto pid : ports) {
        const auto* p = doc.tryPort(pid);
        if (!p)
            continue;
        if (out.north.isNull() && matchesSide(p->name(), PortSide::North)) out.north = pid;
        else if (out.east.isNull() && matchesSide(p->name(), PortSide::East)) out.east = pid;
        else if (out.south.isNull() && matchesSide(p->name(), PortSide::South)) out.south = pid;
        else if (out.west.isNull() && matchesSide(p->name(), PortSide::West)) out.west = pid;
    }

    QSet<DesignModel::PortId> used;
    if (!out.north.isNull()) used.insert(out.north);
    if (!out.east.isNull())  used.insert(out.east);
    if (!out.south.isNull()) used.insert(out.south);
    if (!out.west.isNull())  used.insert(out.west);

    struct Candidate { DesignModel::PortId id; QString name; DesignModel::PortDirection dir; int order = 0; };
    QVector<Candidate> ins;
    QVector<Candidate> outs;
    int order = 0;
    for (const auto pid : ports) {
        if (used.contains(pid))
            continue;
        const auto* p = doc.tryPort(pid);
        if (!p)
            continue;
        Candidate c{pid, p->name(), p->direction(), order++};
        if (p->direction() == DesignModel::PortDirection::Input)
            ins.push_back(c);
        else if (p->direction() == DesignModel::PortDirection::Output)
            outs.push_back(c);
    }

    auto byNameThenOrder = [](const Candidate& a, const Candidate& b) {
        const int cmp = QString::compare(a.name, b.name, Qt::CaseInsensitive);
        if (cmp != 0)
            return cmp < 0;
        return a.order < b.order;
    };
    std::sort(ins.begin(), ins.end(), byNameThenOrder);
    std::sort(outs.begin(), outs.end(), byNameThenOrder);

    auto take = [](QVector<Candidate>& v) -> DesignModel::PortId {
        if (v.isEmpty())
            return {};
        const auto id = v.front().id;
        v.pop_front();
        return id;
    };

    if (out.north.isNull()) out.north = take(ins);
    if (out.west.isNull())  out.west  = take(ins);
    if (out.east.isNull())  out.east  = take(outs);
    if (out.south.isNull()) out.south = take(outs);

    QVector<Candidate> rest;
    rest += ins;
    rest += outs;
    std::sort(rest.begin(), rest.end(), byNameThenOrder);
    if (out.north.isNull()) out.north = take(rest);
    if (out.west.isNull())  out.west  = take(rest);
    if (out.east.isNull())  out.east  = take(rest);
    if (out.south.isNull()) out.south = take(rest);

    return out;
}

const DesignModel::Port* CanvasSceneModel::findPortOnBlockSide(const DesignModel::DesignDocument& doc,
                                                              DesignModel::BlockId blockId,
                                                              PortSide side)
{
    const auto& ports = doc.index().portsForBlock(blockId);
    if (ports.isEmpty())
        return nullptr;

    for (const auto pid : ports) {
        const auto* p = doc.tryPort(pid);
        if (!p)
            continue;
        if (matchesSide(p->name(), side))
            return p;
    }
    return nullptr;
}

QPointF CanvasSceneModel::portCenter(DesignModel::PortId id) const
{
    const auto it = m_portCentersScreen.constFind(id);
    return it == m_portCentersScreen.constEnd() ? QPointF() : *it;
}

QPointF CanvasSceneModel::portCenterWorld(DesignModel::PortId id) const
{
    const auto it = m_portCentersWorld.constFind(id);
    return it == m_portCentersWorld.constEnd() ? QPointF() : *it;
}

static QPointF dirFor(PortSide s)
{
    switch (s) {
    case PortSide::North: return QPointF(0.0, -1.0);
    case PortSide::East:  return QPointF(1.0, 0.0);
    case PortSide::South: return QPointF(0.0, 1.0);
    case PortSide::West:  return QPointF(-1.0, 0.0);
    }
    return QPointF(0.0, 0.0);
}

static QVector<QPointF> simplifyPolyline(QVector<QPointF> pts)
{
    if (pts.size() < 3)
        return pts;
    QVector<QPointF> out;
    out.reserve(pts.size());
    out.push_back(pts.front());
    for (int i = 1; i + 1 < pts.size(); ++i) {
        const bool keep = (i == 1) || (i == pts.size() - 2);
        if (keep) {
            out.push_back(pts[i]);
            continue;
        }

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

static QVector<QRectF> expandObstacles(const QVector<QRectF>& obstacles, double clearance)
{
    QVector<QRectF> out;
    out.reserve(obstacles.size());
    for (const auto& r : obstacles)
        out.push_back(r.adjusted(-clearance, -clearance, clearance, clearance));
    return out;
}

static bool axisContains(const QVector<double>& axis, double v)
{
    const auto it = std::lower_bound(axis.begin(), axis.end(), v);
    if (it == axis.end())
        return false;
    return std::abs(*it - v) < 1e-6;
}

static bool pointInsideObstacle(const QPointF& p, const QRectF& r)
{
    const double eps = 0.25;
    return (p.x() > r.left() + eps && p.x() < r.right() - eps
         && p.y() > r.top() + eps && p.y() < r.bottom() - eps);
}

static bool segmentIntersectsObstacle(const QPointF& a, const QPointF& b, const QRectF& r)
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

    return true;
}

static bool polylineLegal(const QVector<QPointF>& pts,
                          const QVector<double>& xs,
                          const QVector<double>& ys,
                          const QVector<QRectF>& obstaclesExpanded)
{
    if (pts.size() < 2)
        return false;

    for (const auto& p : pts) {
        if (!axisContains(xs, p.x()) || !axisContains(ys, p.y()))
            return false;
        for (const auto& r : obstaclesExpanded) {
            if (pointInsideObstacle(p, r))
                return false;
        }
    }

    for (int i = 0; i + 1 < pts.size(); ++i) {
        const QPointF a = pts[i];
        const QPointF b = pts[i + 1];
        if (!(std::abs(a.x() - b.x()) < 1e-6 || std::abs(a.y() - b.y()) < 1e-6))
            return false;
        for (const auto& r : obstaclesExpanded) {
            if (segmentIntersectsObstacle(a, b, r))
                return false;
        }
    }

    return true;
}

static bool polylineLegalLooseEndpoints(const QVector<QPointF>& pts,
                                       const QVector<double>& xs,
                                       const QVector<double>& ys,
                                       const QVector<QRectF>& obstaclesExpanded)
{
    if (pts.size() < 2)
        return false;

    const int last = pts.size() - 1;
    for (int i = 0; i < pts.size(); ++i) {
        const auto& p = pts[i];
        if (!qIsFinite(p.x()) || !qIsFinite(p.y()))
            return false;

        if (i != 0 && i != last) {
            if (!axisContains(xs, p.x()) || !axisContains(ys, p.y()))
                return false;
            for (const auto& r : obstaclesExpanded) {
                if (pointInsideObstacle(p, r))
                    return false;
            }
        }
    }

    for (int i = 0; i + 1 < pts.size(); ++i) {
        const QPointF a = pts[i];
        const QPointF b = pts[i + 1];
        if (!(std::abs(a.x() - b.x()) < 1e-6 || std::abs(a.y() - b.y()) < 1e-6))
            return false;
        for (const auto& r : obstaclesExpanded) {
            if (segmentIntersectsObstacle(a, b, r))
                return false;
        }
    }

    return true;
}

void CanvasSceneModel::buildFabric(const CanvasViewport& vp, const CanvasRenderOptions& opts)
{
    const double pitch = m_spec.tileSize + m_spec.tileGap;
    const double left = m_spec.margin;
    const double top = m_spec.margin;

    const double halfGap = m_spec.tileGap * 0.5;
    const double stub = std::max(8.0, 3.0 + 2.0);

    const int tracks = std::max(1, m_spec.fabricTracksPerChannel);

    auto insertTracks = [](QSet<double>& axis, double start, double span, int nTracks) {
        if (span <= 0.0)
            return;
        const int n = std::max(1, nTracks);
        for (int k = 1; k <= n; ++k) {
            axis.insert(start + (span * double(k) / double(n + 1)));
        }
    };

    QSet<double> xs;
    QSet<double> ys;

    for (int col = 0; col < m_spec.aieCols; ++col) {
        const double x0 = left + col * pitch;
        const double x1 = x0 + m_spec.tileSize;
        xs.insert(x0);
        xs.insert(x0 + m_spec.tileSize * 0.5);
        xs.insert(x1);
        if (col + 1 < m_spec.aieCols)
            insertTracks(xs, x1, m_spec.tileGap, tracks);
    }
    xs.insert(left - halfGap);
    xs.insert(left + (m_spec.aieCols - 1) * pitch + m_spec.tileSize + halfGap);

    for (int i = 0; i < m_spec.aieRows; ++i) {
        const double y0 = top + i * pitch;
        const double y1 = y0 + m_spec.tileSize;
        ys.insert(y0);
        ys.insert(y0 + m_spec.tileSize * 0.5);
        ys.insert(y1);
        if (i + 1 < m_spec.aieRows)
            insertTracks(ys, y1, m_spec.tileGap, tracks);
    }

    const double computeBottom = top + (m_spec.aieRows - 1) * pitch + m_spec.tileSize;
    insertTracks(ys, computeBottom, m_spec.tileGap + m_spec.bandGap, tracks);

    const double memTopWorld = top + m_spec.aieRows * pitch + m_spec.bandGap;
    for (int r = 0; r < m_spec.memRows; ++r) {
        const double y0 = memTopWorld + r * pitch;
        const double y1 = y0 + m_spec.tileSize;
        ys.insert(y0);
        ys.insert(y0 + m_spec.tileSize * 0.5);
        ys.insert(y1);
        if (r + 1 < m_spec.memRows)
            insertTracks(ys, y1, m_spec.tileGap, tracks);
    }

    const double memBottom = memTopWorld + (m_spec.memRows - 1) * pitch + m_spec.tileSize;
    insertTracks(ys, memBottom, m_spec.tileGap + m_spec.bandGap, tracks);

    const double shimTopWorld = memTopWorld + m_spec.memRows * pitch + m_spec.bandGap;
    for (int r = 0; r < m_spec.shimRows; ++r) {
        const double y0 = shimTopWorld + r * pitch;
        const double y1 = y0 + m_spec.tileSize;
        ys.insert(y0);
        ys.insert(y0 + m_spec.tileSize * 0.5);
        ys.insert(y1);
        if (r + 1 < m_spec.shimRows)
            insertTracks(ys, y1, m_spec.tileGap, tracks);
    }

    for (auto it = m_portCentersWorld.constBegin(); it != m_portCentersWorld.constEnd(); ++it) {
        const DesignModel::PortId pid = it.key();
        const QPointF port = it.value();
        const PortSide side = m_portSides.value(pid, PortSide::East);
        const QPointF out = port + dirFor(side) * stub;

        xs.insert(out.x());
        ys.insert(out.y());

        if (side == PortSide::East || side == PortSide::West)
            ys.insert(port.y());
        else
            xs.insert(port.x());
    }

    m_fabricXs = xs.values();
    m_fabricYs = ys.values();
    std::sort(m_fabricXs.begin(), m_fabricXs.end());
    std::sort(m_fabricYs.begin(), m_fabricYs.end());

    m_fabricObstacles = m_tileWorldRects;

    m_fabricNodes.clear();
    m_fabricEdges.clear();
    if (!opts.showFabric)
        return;

    FabricRouter::Params params;
    params.obstacleClearance = 2.0;

    const FabricOverlay ov = FabricRouter::buildOverlay(m_fabricXs, m_fabricYs, m_fabricObstacles, params);
    m_fabricNodes.reserve(ov.nodes.size());
    for (const QPointF& n : ov.nodes) {
        FabricNodeVisual nv;
        nv.pos = vp.worldToScreen(n);
        m_fabricNodes.push_back(nv);
    }

    m_fabricEdges.reserve(ov.edges.size());
    for (const QLineF& e : ov.edges) {
        FabricEdgeVisual ev;
        ev.line = QLineF(vp.worldToScreen(e.p1()), vp.worldToScreen(e.p2()));
        m_fabricEdges.push_back(ev);
    }
}

void CanvasSceneModel::buildLinks(const DesignModel::DesignDocument& doc, const CanvasViewport& vp)
{
    if (m_fabricXs.isEmpty() || m_fabricYs.isEmpty())
        return;

    FabricRouter::Params params;
    params.obstacleClearance = 2.0;
    const double stub = std::max(8.0, params.obstacleClearance + 3.0);

    const QVector<QRectF> obstaclesExpanded = expandObstacles(m_fabricObstacles, params.obstacleClearance);

    for (const auto lid : doc.linkIds()) {
        const auto* l = doc.tryLink(lid);
        if (!l || !l->isValid())
            continue;

        const QPointF aPort = portCenterWorld(l->from());
        const QPointF bPort = portCenterWorld(l->to());
        if (aPort.isNull() || bPort.isNull())
            continue;

        QVector<QPointF> worldPts;

        const PortSide aSide = m_portSides.value(l->from(), PortSide::East);
        const PortSide bSide = m_portSides.value(l->to(), PortSide::West);
        const QPointF aOut = aPort + dirFor(aSide) * stub;
        const QPointF bOut = bPort + dirFor(bSide) * stub;

        const auto itPreview = m_linkRoutePreviewWorld.constFind(lid);
        if (itPreview != m_linkRoutePreviewWorld.constEnd() && itPreview.value().size() >= 2) {
            worldPts = itPreview.value();
            worldPts.front() = aPort;
            worldPts.back() = bPort;
            worldPts = simplifyPolyline(std::move(worldPts));
        }

        if (worldPts.isEmpty() && l->hasRouteOverride()) {
            const auto& ov = l->routeOverride().value();
            if (ov.isValid() && ov.isAuthoritative()) {
                const QVector<QPointF> mid = ov.waypointsWorld(); // does not include ports
                if (!mid.isEmpty()) {
                    QVector<QPointF> pts;
                    pts.reserve(mid.size() + 4);
                    pts.push_back(aPort);
                    pts.push_back(aOut);
                    for (const auto& p : mid)
                        pts.push_back(p);
                    pts.push_back(bOut);
                    pts.push_back(bPort);
                    pts = simplifyPolyline(std::move(pts));

                    if (pts.size() >= 2
                        && polylineLegalLooseEndpoints(pts, m_fabricXs, m_fabricYs, obstaclesExpanded)) {
                        worldPts = std::move(pts);
                    }
                }
            }
        }

        if (worldPts.isEmpty()) {
            const QVector<QPointF> mid = FabricRouter::route(aOut, bOut, m_fabricXs, m_fabricYs, m_fabricObstacles, params);

            auto appendNoDup = [&](const QPointF& p) {
                if (!worldPts.isEmpty()) {
                    const QPointF& last = worldPts.back();
                    if (std::abs(last.x() - p.x()) < 1e-6 && std::abs(last.y() - p.y()) < 1e-6)
                        return;
                }
                worldPts.push_back(p);
            };

            appendNoDup(aPort);
            appendNoDup(aOut);
            for (const QPointF& p : mid)
                appendNoDup(p);
            appendNoDup(bOut);
            appendNoDup(bPort);
            worldPts = simplifyPolyline(std::move(worldPts));
        }

        LinkVisual lv;
        lv.id = lid;
        lv.from = l->from();
        lv.to = l->to();
        lv.worldPoints = worldPts;
        lv.points.reserve(worldPts.size());
        for (const QPointF& wp : worldPts)
            lv.points.push_back(vp.worldToScreen(wp));
        m_links.push_back(std::move(lv));
    }
}

void CanvasSceneModel::buildAnnotations(const DesignModel::DesignDocument& doc,
                                       const CanvasRenderOptions& opts)
{
    if (!opts.showAnnotations)
        return;

    for (const auto aid : doc.annotationIds()) {
        const auto* ann = doc.tryAnnotation(aid);
        if (!ann)
            continue;
        if (!ann->isValid())
            continue;

        QRectF anchor;
        if (!ann->tileTargets().isEmpty()) {
            anchor = computeTileRect(ann->tileTargets().front());
        } else if (!ann->blockTargets().isEmpty()) {
            const auto* b = doc.tryBlock(ann->blockTargets().front());
            if (b)
                anchor = computeTileRect(b->placement().anchor());
        }
        if (anchor.isEmpty())
            continue;

        AnnotationVisual av;
        av.anchorRect = anchor;
        av.text = ann->text();
        m_annotations.push_back(av);
    }
}

void CanvasSceneModel::buildHotspots(const DesignModel::DesignDocument& doc, const CanvasViewport& vp)
{
    m_hotspots.reserve(m_tiles.size() * 4);
    for (const auto& tv : m_tiles) {
        const QRectF r = tv.rect;
        const double s = std::min(r.width(), r.height()) * 0.14;
        const double cx = r.center().x();
        const double cy = r.center().y();

        auto add = [&](PortSide side, const QRectF& rr) {
            PortHotspot h;
            h.rect = rr;
            h.tileKind = tv.kind;
            h.tileCoord = tv.coord;
            h.side = side;

            if (tv.kind == DesignModel::TileKind::Aie) {
                for (const auto bid : doc.blockIds()) {
                    const auto* b = doc.tryBlock(bid);
                    if (!b)
                        continue;
                    if (b->placement().anchor() != tv.coord)
                        continue;

                    const SidePorts portsBySide = assignPortsForSides(doc, bid);
                    h.portId = portsBySide.forSide(side);
                    if (!h.portId.isNull()) {
                        const QPointF centerScreen = rr.center();
                        m_portCentersScreen.insert(h.portId, centerScreen);
                        m_portCentersWorld.insert(h.portId, vp.screenToWorld(centerScreen));
                        m_portSides.insert(h.portId, side);
                        m_portAnchors.insert(h.portId, tv.coord);
                    }
                    break;
                }
            }
            m_hotspots.push_back(h);
        };

        add(PortSide::North, QRectF(cx - s/2.0, r.top() - s/2.0, s, s));
        add(PortSide::South, QRectF(cx - s/2.0, r.bottom() - s/2.0, s, s));
        add(PortSide::West,  QRectF(r.left() - s/2.0, cy - s/2.0, s, s));
        add(PortSide::East,  QRectF(r.right() - s/2.0, cy - s/2.0, s, s));
    }
}

QRectF CanvasSceneModel::computeTileRect(DesignModel::TileCoord coord) const
{
    const auto it = m_computeRectsScreen.constFind(keyFor(coord));
    return it == m_computeRectsScreen.constEnd() ? QRectF() : *it;
}

} // namespace Canvas
