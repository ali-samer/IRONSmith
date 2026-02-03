#include "designmodel/DesignSchemaVersion.hpp"

#include <QtCore/QStringView>

namespace DesignModel {

QString DesignSchemaVersion::toString() const {
	return QString::number(m_value);
}

std::optional<DesignSchemaVersion> DesignSchemaVersion::fromString(const QString& s) {
	QStringView v = QStringView{s}.trimmed();
	if (v.isEmpty())
		return std::nullopt;

	// accepts "v1" or "V1" as well as "1"
	if (v.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
		v = v.mid(1).trimmed();

	bool ok = false;
	const quint64 parsed = v.toULongLong(&ok, 10);
	if (!ok || parsed == 0 || parsed > std::numeric_limits<value_type>::max())
		return std::nullopt;

	return DesignSchemaVersion{static_cast<value_type>(parsed)};
}

} // namespace DesignModel