#pragma once

#include <QtCore/QLineF>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <designmodel/DesignId.hpp>
#include <designmodel/Tile.hpp>

namespace Canvas {

enum class PortSide : quint8 { North, East, South, West };

struct TileVisual
{
    DesignModel::TileKind kind = DesignModel::TileKind::Unknown;
    DesignModel::TileCoord coord{};
    QRectF rect{};
    QString label;
};

struct PortHotspot
{
    QRectF rect{};                 // screen space hitbox
    DesignModel::TileKind tileKind = DesignModel::TileKind::Unknown;
    DesignModel::TileCoord tileCoord{};
    PortSide side = PortSide::North;

    DesignModel::PortId portId{};
};

struct LinkVisual
{
    DesignModel::LinkId id{};
    DesignModel::PortId from{};
    DesignModel::PortId to{};
    QVector<QPointF> worldPoints; // world space polyline (stable under zoom/pan)
    QVector<QPointF> points; // screen space polyline
};

struct AnnotationVisual
{
    QRectF anchorRect{}; // near which we render
    QString text;
};

struct BlockVisual
{
    DesignModel::BlockId id{};
    QRectF rect{};
    QString text;
};

struct FabricNodeVisual
{
    QPointF pos; // screen space
};

struct FabricEdgeVisual
{
    QLineF line; // screen space
};

} // namespace Canvas
