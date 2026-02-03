#pragma once

#include "designmodel/DesignModelGlobal.hpp"
#include "designmodel/DesignId.hpp"
#include "designmodel/Tile.hpp"

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QPointF>

#include <optional>
#include <compare>

namespace DesignModel {

enum class BlockType : quint8 {
    Compute,
    Memory,
    ShimInterface,
    Ddr,
    Unknown
};

enum class PortDirection : quint8 {
    Input,
    Output,
    InOut
};

enum class PortTypeKind : quint8 {
    Stream,
    Packet,
    Dma,
    Control,
    Unknown
};

class DESIGNMODEL_EXPORT PortType final {
public:
    PortType() = default;
    explicit PortType(PortTypeKind kind, QString payload = {})
        : m_kind(kind), m_payload(std::move(payload)) {}

    PortTypeKind kind() const noexcept { return m_kind; }
    const QString& payload() const noexcept { return m_payload; }

    bool isValid() const noexcept { return m_kind != PortTypeKind::Unknown; }

    friend bool operator==(const PortType&, const PortType&) noexcept = default;
    friend std::strong_ordering operator<=>(const PortType& a, const PortType& b) noexcept {
        if (a.m_kind != b.m_kind)
            return (a.m_kind < b.m_kind) ? std::strong_ordering::less : std::strong_ordering::greater;

        const int c = QString::compare(a.m_payload, b.m_payload, Qt::CaseSensitive);
        if (c < 0) return std::strong_ordering::less;
        if (c > 0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

private:
    PortTypeKind m_kind{PortTypeKind::Unknown};
    QString m_payload;
};

class DESIGNMODEL_EXPORT Placement final {
public:
    Placement() = default;
    explicit Placement(TileCoord anchor, int rowSpan = 1, int colSpan = 1)
        : m_anchor(anchor), m_rowSpan(rowSpan), m_colSpan(colSpan) {}

    const TileCoord& anchor() const noexcept { return m_anchor; }
    int rowSpan() const noexcept { return m_rowSpan; }
    int colSpan() const noexcept { return m_colSpan; }

    bool isValid() const noexcept {
        return m_anchor.isValid() && m_rowSpan >= 1 && m_colSpan >= 1;
    }

    friend bool operator==(const Placement&, const Placement&) noexcept = default;

private:
    TileCoord m_anchor{};
    int m_rowSpan{1};
    int m_colSpan{1};
};

class DESIGNMODEL_EXPORT Block final {
public:
    Block() = default;
    Block(BlockId id, BlockType type, Placement placement, QString displayName = {})
        : m_id(id)
        , m_type(type)
        , m_placement(std::move(placement))
        , m_displayName(std::move(displayName)) {}

    BlockId id() const noexcept { return m_id; }
    BlockType type() const noexcept { return m_type; }
    const Placement& placement() const noexcept { return m_placement; }
    const QString& displayName() const noexcept { return m_displayName; }

    const QVector<PortId>& ports() const noexcept { return m_ports; }

    bool isValid() const noexcept {
        return !m_id.isNull() && m_type != BlockType::Unknown && m_placement.isValid();
    }

    void addPort(PortId p) { m_ports.push_back(p); }

private:
    BlockId m_id{};
    BlockType m_type{BlockType::Unknown};
    Placement m_placement{};
    QString m_displayName;
    QVector<PortId> m_ports;
};

class DESIGNMODEL_EXPORT Port final {
public:
    Port() = default;
    Port(PortId id,
         BlockId owner,
         PortDirection dir,
         PortType type,
         QString name = {},
         int capacity = 1)
        : m_id(id)
        , m_owner(owner)
        , m_direction(dir)
        , m_type(std::move(type))
        , m_name(std::move(name))
        , m_capacity(capacity) {}

    PortId id() const noexcept { return m_id; }
    BlockId owner() const noexcept { return m_owner; }
    PortDirection direction() const noexcept { return m_direction; }
    const PortType& type() const noexcept { return m_type; }
    const QString& name() const noexcept { return m_name; }
    int capacity() const noexcept { return m_capacity; }

    bool isValid() const noexcept {
        return !m_id.isNull()
            && !m_owner.isNull()
            && m_type.isValid()
            && m_capacity >= 1;
    }

private:
    PortId m_id{};
    BlockId m_owner{};
    PortDirection m_direction{PortDirection::Input};
    PortType m_type{};
    QString m_name;
    int m_capacity{1};
};


class DESIGNMODEL_EXPORT RouteOverride final {
public:
    RouteOverride() = default;
    explicit RouteOverride(QVector<QPointF> waypointsWorld, bool authoritative = true)
        : m_waypointsWorld(std::move(waypointsWorld))
        , m_authoritative(authoritative) {}

    const QVector<QPointF>& waypointsWorld() const noexcept { return m_waypointsWorld; }
    bool isAuthoritative() const noexcept { return m_authoritative; }

    bool isValid() const noexcept {
        if (m_waypointsWorld.isEmpty())
            return false;
        for (const auto& p : m_waypointsWorld) {
            if (!qIsFinite(p.x()) || !qIsFinite(p.y()))
                return false;
        }
        return true;
    }

    friend bool operator==(const RouteOverride&, const RouteOverride&) noexcept = default;

private:
    QVector<QPointF> m_waypointsWorld;
    bool m_authoritative = true;
};

class DESIGNMODEL_EXPORT Link final {
public:
    Link() = default;
    Link(LinkId id, PortId from, PortId to, QString label = {}, std::optional<RouteOverride> routeOverride = {})
        : m_id(id)
        , m_from(from)
        , m_to(to)
        , m_label(std::move(label))
        , m_routeOverride(std::move(routeOverride)) {}

    LinkId id() const noexcept { return m_id; }
    PortId from() const noexcept { return m_from; }
    PortId to() const noexcept { return m_to; }
    const QString& label() const noexcept { return m_label; }
    const std::optional<RouteOverride>& routeOverride() const noexcept { return m_routeOverride; }
    bool hasRouteOverride() const noexcept { return m_routeOverride.has_value(); }

    bool isValid() const noexcept {
        return !m_id.isNull() && !m_from.isNull() && !m_to.isNull() && (m_from != m_to);
    }

private:
    LinkId m_id{};
    PortId m_from{};
    PortId m_to{};
    QString m_label;
    std::optional<RouteOverride> m_routeOverride;
};

} // namespace DesignModel