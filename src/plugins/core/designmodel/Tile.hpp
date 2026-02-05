#pragma once

#include "designmodel/DesignModelGlobal.hpp"

#include <QtCore/QString>
#include <QtCore/QHashFunctions>

#include <optional>
#include <compare>

namespace DesignModel {

enum class TileKind : quint8 {
	Aie,
	Mem,
	Shim,
	Unknown
};

DESIGNMODEL_EXPORT QString toString(TileKind kind);
DESIGNMODEL_EXPORT std::optional<TileKind> tileKindFromString(const QString& s);

class DESIGNMODEL_EXPORT TileCoord final {
public:
	constexpr TileCoord() noexcept = default;
	constexpr TileCoord(int row, int col) noexcept : m_row(row), m_col(col) {}

	constexpr int row() const noexcept { return m_row; }
	constexpr int col() const noexcept { return m_col; }

	constexpr bool isValid() const noexcept { return m_row >= 0 && m_col >= 0; }

	QString toString() const;
	static std::optional<TileCoord> fromString(const QString& s);

	friend bool operator==(const TileCoord&, const TileCoord&) noexcept = default;
	friend std::strong_ordering operator<=>(const TileCoord& a, const TileCoord& b) noexcept {
		if (a.m_row != b.m_row)
			return (a.m_row < b.m_row) ? std::strong_ordering::less : std::strong_ordering::greater;
		if (a.m_col != b.m_col)
			return (a.m_col < b.m_col) ? std::strong_ordering::less : std::strong_ordering::greater;
		return std::strong_ordering::equal;
	}

private:
	int m_row{-1};
	int m_col{-1};
};

inline size_t qHash(const TileCoord& c, size_t seed = 0) noexcept {
	seed ^= ::qHash(c.row(), seed);
	seed ^= ::qHash(c.col(), seed << 1);
	return seed;
}

} // namespace DesignModel