#pragma once

#include "designmodel/DesignModelGlobal.hpp"
#include "designmodel/DesignSchemaVersion.hpp"
#include "designmodel/DesignMetadata.hpp"
#include "designmodel/DesignEntities.hpp"
#include "designmodel/DesignIndex.hpp"
#include "designmodel/DesignExtras.hpp"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtCore/QSharedData>
#include <QtCore/QExplicitlySharedDataPointer>

namespace DesignModel {

class DESIGNMODEL_EXPORT DesignDocument final {
public:
    DesignDocument();
    DesignDocument(const DesignDocument&) = default;
    DesignDocument& operator=(const DesignDocument&) = default;
    ~DesignDocument();

    const DesignSchemaVersion& schemaVersion() const noexcept;
    const DesignMetadata& metadata() const noexcept;
    const DesignIndex& index() const noexcept;

    // iteration
    const QVector<BlockId>& blockIds() const noexcept;
    const QVector<PortId>&  portIds() const noexcept;
    const QVector<LinkId>&  linkIds() const noexcept;

    const QVector<NetId>&        netIds() const noexcept;
    const QVector<AnnotationId>& annotationIds() const noexcept;
    const QVector<RouteId>&      routeIds() const noexcept;

    // Lookup
    const Block* tryBlock(BlockId id) const noexcept;
    const Port*  tryPort(PortId id) const noexcept;
    const Link*  tryLink(LinkId id) const noexcept;

    const Net*        tryNet(NetId id) const noexcept;
    const Annotation* tryAnnotation(AnnotationId id) const noexcept;
    const Route*      tryRoute(RouteId id) const noexcept;

    bool isValid() const noexcept;

    class DESIGNMODEL_EXPORT Builder final {
    public:
        Builder(DesignSchemaVersion v, DesignMetadata md);
        explicit Builder(const DesignDocument& document);

        BlockId createBlock(BlockType type, Placement placement, QString displayName = {});
        PortId  createPort(BlockId owner,
                           PortDirection dir,
                           PortType type,
                           QString name = {},
                           int capacity = 1);
        LinkId  createLink(PortId from, PortId to, QString label = {});

        bool setLinkRouteOverride(LinkId id, std::optional<RouteOverride> routeOverride);

        NetId createNet(QString name = {}, QVector<LinkId> links = {});
        AnnotationId createAnnotation(AnnotationKind kind,
                                      QString text,
                                      QVector<BlockId> blocks = {},
                                      QVector<PortId> ports = {},
                                      QVector<LinkId> links = {},
                                      QVector<TileCoord> tiles = {},
                                      QString tag = {});
        RouteId createRoute(LinkId link, QVector<TileCoord> path = {});

        bool removeLink(LinkId id);
        bool removeBlock(BlockId id);
        bool removeAnnotation(AnnotationId id);
        bool removeNet(NetId id);
        bool removeRoute(RouteId id);

        DesignDocument freeze() const;

    private:
        DesignSchemaVersion m_version;
        DesignMetadata m_metadata;

        QHash<BlockId, Block> m_blocks;
        QHash<PortId, Port>   m_ports;
        QHash<LinkId, Link>   m_links;

        QHash<NetId, Net> m_nets;
        QHash<AnnotationId, Annotation> m_annotations;
        QHash<RouteId, Route> m_routes;

        QVector<BlockId> m_blockOrder;
        QVector<PortId>  m_portOrder;
        QVector<LinkId>  m_linkOrder;

        QVector<NetId> m_netOrder;
        QVector<AnnotationId> m_annotationOrder;
        QVector<RouteId> m_routeOrder;
    };

private:
    struct Data final : public QSharedData {
        DesignSchemaVersion version;
        DesignMetadata metadata;
        DesignIndex index;

        QHash<BlockId, Block> blocks;
        QHash<PortId, Port>   ports;
        QHash<LinkId, Link>   links;

        QHash<NetId, Net> nets;
        QHash<AnnotationId, Annotation> annotations;
        QHash<RouteId, Route> routes;

        QVector<BlockId> blockOrder;
        QVector<PortId>  portOrder;
        QVector<LinkId>  linkOrder;

        QVector<NetId> netOrder;
        QVector<AnnotationId> annotationOrder;
        QVector<RouteId> routeOrder;

        bool isValid() const noexcept;
    };

    explicit DesignDocument(QExplicitlySharedDataPointer<Data> data);

    QExplicitlySharedDataPointer<Data> d;
};

} // namespace DesignModel
