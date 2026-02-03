#pragma once

#include "canvas/CanvasRenderOptions.hpp"
#include "canvas/CanvasPrimitives.hpp"
#include "canvas/CanvasViewport.hpp"

#include <designmodel/DesignDocument.hpp>

#include <QtCore/QHash>
#include <QtCore/QVector>

namespace Canvas {

struct GridSpec
{
    int aieCols = 8;
    int aieRows = 6;
    int memRows = 1;  // band below compute
    int shimRows = 1; // band below mem

    int fabricTracksPerChannel = 3;

    double tileSize = 70.0;  // world units @ zoom=1
    double tileGap = 86.0;
    double bandGap = 36.0;
    double margin = 40.0;
};

class CanvasSceneModel final
{
public:
    CanvasSceneModel();

    const GridSpec& gridSpec() const noexcept { return m_spec; }
    void setGridSpec(GridSpec spec);

    void rebuild(const DesignModel::DesignDocument& doc,
                 const CanvasViewport& vp,
                 const CanvasRenderOptions& opts);

    const QVector<TileVisual>& tiles() const noexcept { return m_tiles; }
    const QVector<PortHotspot>& hotspots() const noexcept { return m_hotspots; }
    const QVector<BlockVisual>& blocks() const noexcept { return m_blocks; }
    const QVector<LinkVisual>&  links() const noexcept { return m_links; }
    const QVector<AnnotationVisual>& annotations() const noexcept { return m_annotations; }
    const QVector<FabricNodeVisual>& fabricNodes() const noexcept { return m_fabricNodes; }
    const QVector<FabricEdgeVisual>& fabricEdges() const noexcept { return m_fabricEdges; }

    QRectF computeTileRect(DesignModel::TileCoord coord) const;

    QPointF portCenter(DesignModel::PortId id) const;
    QPointF portCenterWorld(DesignModel::PortId id) const;

    const QVector<double>& fabricXs() const noexcept { return m_fabricXs; }
    const QVector<double>& fabricYs() const noexcept { return m_fabricYs; }
    const QVector<QRectF>& fabricObstacles() const noexcept { return m_fabricObstacles; }

    void setLinkRoutePreview(DesignModel::LinkId id, QVector<QPointF> worldPolyline);
    void clearLinkRoutePreview(DesignModel::LinkId id);
    void clearAllLinkRoutePreviews();

private:
    QRectF worldRectToScreen(const QRectF& world, const CanvasViewport& vp) const;
    void buildTiles(const CanvasViewport& vp);
    void buildBlocks(const DesignModel::DesignDocument& doc);
    void buildFabric(const CanvasViewport& vp, const CanvasRenderOptions& opts);
    void buildLinks(const DesignModel::DesignDocument& doc, const CanvasViewport& vp);
    void buildAnnotations(const DesignModel::DesignDocument& doc, const CanvasRenderOptions& opts);
    void buildHotspots(const DesignModel::DesignDocument& doc, const CanvasViewport& vp);

    const DesignModel::Port* findPortOnBlockSide(const DesignModel::DesignDocument& doc,
                                                                  DesignModel::BlockId blockId,
                                                                  PortSide side);

private:
    GridSpec m_spec;
    QVector<TileVisual> m_tiles;
    QVector<PortHotspot> m_hotspots;
    QVector<BlockVisual> m_blocks;
    QVector<LinkVisual> m_links;
    QVector<AnnotationVisual> m_annotations;

    QVector<FabricNodeVisual> m_fabricNodes;
    QVector<FabricEdgeVisual> m_fabricEdges;

    QVector<double> m_fabricXs;
    QVector<double> m_fabricYs;
    QVector<QRectF> m_fabricObstacles;
    QVector<QRectF> m_tileWorldRects;

    QHash<DesignModel::LinkId, QVector<QPointF>> m_linkRoutePreviewWorld;

    QHash<quint64, QRectF> m_computeRectsScreen;
    QHash<quint64, QRectF> m_computeRectsWorld;

    QHash<DesignModel::PortId, QPointF> m_portCentersScreen;
    QHash<DesignModel::PortId, QPointF> m_portCentersWorld;
    QHash<DesignModel::PortId, PortSide> m_portSides;
    QHash<DesignModel::PortId, DesignModel::TileCoord> m_portAnchors;
};

} // namespace Canvas
