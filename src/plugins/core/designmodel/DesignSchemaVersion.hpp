#pragma once

#include "designmodel/DesignModelGlobal.hpp"

#include <QtCore/QString>
#include <QtCore/QHashFunctions>

#include <optional>
#include <compare>

namespace DesignModel {

class DESIGNMODEL_EXPORT DesignSchemaVersion final {
public:
    using value_type = quint32;

    constexpr DesignSchemaVersion() noexcept = default;
    explicit constexpr DesignSchemaVersion(value_type v) noexcept : m_value(v) {}

    static constexpr DesignSchemaVersion current() noexcept { return DesignSchemaVersion{kCurrentValue}; }
    static constexpr DesignSchemaVersion invalid() noexcept { return DesignSchemaVersion{0}; }

    constexpr bool isValid() const noexcept { return m_value != 0; }
    constexpr value_type value() const noexcept { return m_value; }

    constexpr bool isSupported() const noexcept {
        return isValid() && m_value >= kMinSupportedValue && m_value <= kCurrentValue;
    }

    constexpr bool requiresMigration() const noexcept {
        return isValid() && m_value < kCurrentValue;
    }

    QString toString() const; // e.g. "1"
    static std::optional<DesignSchemaVersion> fromString(const QString& s);

    friend bool operator==(const DesignSchemaVersion&, const DesignSchemaVersion&) noexcept = default;
    friend std::strong_ordering operator<=>(const DesignSchemaVersion& a,
                                            const DesignSchemaVersion& b) noexcept {
        if (a.m_value < b.m_value) return std::strong_ordering::less;
        if (a.m_value > b.m_value) return std::strong_ordering::greater;
        return std::strong_ordering::equal;
    }

private:
    value_type m_value{0};

    static constexpr value_type kCurrentValue = 1;
    static constexpr value_type kMinSupportedValue = 1;
};

inline size_t qHash(const DesignSchemaVersion& v, size_t seed = 0) noexcept {
    return ::qHash(v.value(), seed);
}

} // namespace DesignModel
