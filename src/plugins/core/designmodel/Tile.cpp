#include "designmodel/Tile.hpp"

#include <QtCore/QStringView>

namespace DesignModel {

QString toString(TileKind kind) {
	switch (kind) {
		case TileKind::Aie:     return QStringLiteral("AIE");
		case TileKind::Mem:     return QStringLiteral("MEM");
		case TileKind::Shim:    return QStringLiteral("SHIM");
		case TileKind::Unknown: return QStringLiteral("UNKNOWN");
	}
	return QStringLiteral("UNKNOWN");
}


std::optional<TileKind> tileKindFromString(const QString& s) {
	const QStringView v = QStringView{s}.trimmed();

	auto eq = [&](QLatin1StringView lit) {
		return v.compare(lit, Qt::CaseInsensitive) == 0;
	};

	if (eq(QLatin1StringView("AIE")))     return TileKind::Aie;
	if (eq(QLatin1StringView("MEM")))     return TileKind::Mem;
	if (eq(QLatin1StringView("SHIM")))    return TileKind::Shim;
	if (eq(QLatin1StringView("UNKNOWN"))) return TileKind::Unknown;

	return std::nullopt;
}

QString TileCoord::toString() const {
	return QString::number(m_row) + QLatin1Char(',') + QString::number(m_col);
}

std::optional<TileCoord> TileCoord::fromString(const QString& s) {
	const auto v = QStringView{s}.trimmed();
	const auto comma = v.indexOf(QLatin1Char(','));
	if (comma <= 0 || comma >= v.size() - 1)
		return std::nullopt;

	bool ok1 = false;
	bool ok2 = false;

	const int r = v.left(comma).toInt(&ok1);
	const int c = v.mid(comma + 1).toInt(&ok2);

	if (!ok1 || !ok2)
		return std::nullopt;

	TileCoord tc(r, c);
	if (!tc.isValid())
		return std::nullopt;

	return tc;
}

} // namespace DesignModel
