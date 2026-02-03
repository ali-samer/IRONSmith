#pragma once

#include "designmodel/DesignModelGlobal.hpp"

#include <QtCore/QUuid>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QHashFunctions>

#include <optional>
#include <compare>

namespace DesignModel {

namespace Internal {

inline QByteArray uuidBytes(const QUuid& u) { return u.toRfc4122(); }

inline int compareUuidBytes(const QUuid& a, const QUuid& b) {
    const auto ab = uuidBytes(a);
    const auto bb = uuidBytes(b);
    const int n = std::min(ab.size(), bb.size());
    for (int i = 0; i < n; ++i) {
        const unsigned char ac = static_cast<unsigned char>(ab[i]);
        const unsigned char bc = static_cast<unsigned char>(bb[i]);
        if (ac < bc) return -1;
        if (ac > bc) return 1;
    }
    if (ab.size() < bb.size()) return -1;
    if (ab.size() > bb.size()) return 1;
    return 0;
}

inline std::optional<QUuid> parseUuidLenient(QString s) {
    s = s.trimmed();
    if (s.isEmpty())
        return std::nullopt;

    QUuid u = QUuid::fromString(s);
    if (!u.isNull())
        return u;

    if (!s.startsWith(u'{'))
        s.prepend(u'{');
    if (!s.endsWith(u'}'))
        s.append(u'}');

    u = QUuid::fromString(s);
    if (u.isNull())
        return std::nullopt;

    return u;
}

} // namespace Internal

template <typename Tag>
class DESIGNMODEL_EXPORT StrongId final {
public:
    using tag_type = Tag;

    constexpr StrongId() noexcept = default;
    explicit StrongId(const QUuid& uuid) noexcept : m_uuid(uuid) {}

    static StrongId create() { return StrongId(QUuid::createUuid()); }
    static constexpr StrongId null() noexcept { return StrongId(); }

    bool isNull() const noexcept { return m_uuid.isNull(); }
    const QUuid& uuid() const noexcept { return m_uuid; }

    QString toString(QUuid::StringFormat fmt = QUuid::WithoutBraces) const {
        return m_uuid.toString(fmt);
    }

    static std::optional<StrongId> fromString(const QString& s) {
        const auto parsed = Internal::parseUuidLenient(s);
        if (!parsed)
            return std::nullopt;
        return StrongId(*parsed);
    }

    friend bool operator==(const StrongId& a, const StrongId& b) noexcept {
        return a.m_uuid == b.m_uuid;
    }

    friend std::strong_ordering operator<=>(const StrongId& a, const StrongId& b) noexcept {
        const int c = Internal::compareUuidBytes(a.m_uuid, b.m_uuid);
        if (c < 0) return std::strong_ordering::less;
        if (c > 0) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

private:
    QUuid m_uuid{};
};

template <typename Tag>
size_t qHash(const StrongId<Tag>& id, size_t seed = 0) noexcept {
    return qHash(id.uuid(), seed);
}

// Typed IDs used by the design graph.
struct BlockIdTag final {};
struct PortIdTag  final {};
struct LinkIdTag  final {};
struct NetIdTag   final {};
struct AnnotationIdTag final {};
struct RouteIdTag final {};

using BlockId = StrongId<BlockIdTag>;
using PortId  = StrongId<PortIdTag>;
using LinkId  = StrongId<LinkIdTag>;
using NetId   = StrongId<NetIdTag>;
using AnnotationId = StrongId<AnnotationIdTag>;
using RouteId      = StrongId<RouteIdTag>;

} // namespace DesignModel