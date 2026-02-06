#pragma once

#include "designmodel/DesignModelGlobal.hpp"
#include "designmodel/DesignId.hpp"
#include "designmodel/Tile.hpp"

#include <QtCore/QString>
#include <QtCore/QVector>

namespace DesignModel {

enum class AnnotationKind : quint8 {
    Label,
    Note,
    Tag,
    Unknown
};

class DESIGNMODEL_EXPORT Annotation final {
public:
    Annotation() = default;

    Annotation(AnnotationId id,
               AnnotationKind kind,
               QString text,
               QVector<BlockId> blocks = {},
               QVector<PortId> ports = {},
               QVector<LinkId> links = {},
               QVector<TileCoord> tiles = {},
               QString tag = {})
        : m_id(id)
        , m_kind(kind)
        , m_text(std::move(text))
        , m_blocks(std::move(blocks))
        , m_ports(std::move(ports))
        , m_links(std::move(links))
        , m_tiles(std::move(tiles))
        , m_tag(std::move(tag)) {}

    AnnotationId id() const noexcept { return m_id; }
    AnnotationKind kind() const noexcept { return m_kind; }
    const QString& text() const noexcept { return m_text; }

    const QVector<BlockId>& blockTargets() const noexcept { return m_blocks; }
    const QVector<PortId>&  portTargets() const noexcept { return m_ports; }
    const QVector<LinkId>&  linkTargets() const noexcept { return m_links; }
    const QVector<TileCoord>& tileTargets() const noexcept { return m_tiles; }

    const QString& tag() const noexcept { return m_tag; }

    bool isValid() const noexcept {
        return !m_id.isNull() && m_kind != AnnotationKind::Unknown && !m_text.isEmpty();
    }

private:
    AnnotationId m_id{};
    AnnotationKind m_kind{AnnotationKind::Unknown};
    QString m_text;

    QVector<BlockId> m_blocks;
    QVector<PortId>  m_ports;
    QVector<LinkId>  m_links;
    QVector<TileCoord> m_tiles;

    QString m_tag;
};

class DESIGNMODEL_EXPORT Net final {
public:
    Net() = default;

    Net(NetId id, QString name = {}, QVector<LinkId> links = {})
        : m_id(id), m_name(std::move(name)), m_links(std::move(links)) {}

    NetId id() const noexcept { return m_id; }
    const QString& name() const noexcept { return m_name; }
    const QVector<LinkId>& links() const noexcept { return m_links; }

    bool isValid() const noexcept { return !m_id.isNull(); }

private:
    NetId m_id{};
    QString m_name;
    QVector<LinkId> m_links;
};

class DESIGNMODEL_EXPORT Route final {
public:
    Route() = default;

    Route(RouteId id, LinkId link, QVector<TileCoord> path = {})
        : m_id(id), m_link(link), m_path(std::move(path)) {}

    RouteId id() const noexcept { return m_id; }
    LinkId link() const noexcept { return m_link; }
    const QVector<TileCoord>& path() const noexcept { return m_path; }

    bool isValid() const noexcept { return !m_id.isNull() && !m_link.isNull(); }

private:
    RouteId m_id{};
    LinkId m_link{};
    QVector<TileCoord> m_path;
};

} // namespace DesignModel
