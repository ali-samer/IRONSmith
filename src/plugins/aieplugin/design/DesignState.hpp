#pragma once

#include "canvas/CanvasPorts.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QString>
#include <QtCore/QVector>

namespace Aie::Internal {

enum class DesignNodeKind : unsigned char {
    Tile,
    LinkHub
};

enum class DesignLinkHubKind : unsigned char {
    Split,
    Join,
    Broadcast
};

struct DesignPort final {
    Canvas::PortSide side = Canvas::PortSide::Left;
    Canvas::PortRole role = Canvas::PortRole::Dynamic;
    double t = 0.5;
    QString name;
    bool hasName = false;
    QString pairId;
    bool hasPairId = false;
};

struct DesignEndpoint final {
    QString nodeId;
    DesignPort port;
};

struct DesignNode final {
    QString id;
    DesignNodeKind kind = DesignNodeKind::Tile;

    Canvas::GridCoord coord{};
    bool hasCoord = false;

    QString kernelRef;
    bool hasKernelRef = false;

    DesignLinkHubKind hubKind = DesignLinkHubKind::Split;
    bool hasHubKind = false;

    QRectF bounds;
    bool hasBounds = false;
};

struct DesignLink final {
    QString id;
    DesignEndpoint from;
    DesignEndpoint to;
    QVector<Canvas::FabricCoord> routeOverride;
};

struct DesignView final {
    double zoom = 1.0;
    QPointF pan{0.0, 0.0};
};

struct DesignState final {
    int schemaVersion = 1;
    DesignView view;
    QVector<DesignNode> nodes;
    QVector<DesignLink> links;
    QJsonObject metadata;
};

} // namespace Aie::Internal
