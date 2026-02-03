#include "designmodel/DesignMetadata.hpp"

namespace DesignModel {

QDateTime DesignMetadata::normalizeUtc(QDateTime dt) {
	if (!dt.isValid())
		return dt;

	dt = dt.toUTC();
	return dt;
}

DesignMetadata::DesignMetadata(QString name,
							   QString author,
							   QDateTime createdUtc,
							   QString notes,
							   QString profileSignature)
	: m_name(std::move(name))
	, m_author(std::move(author))
	, m_createdUtc(normalizeUtc(std::move(createdUtc)))
	, m_notes(std::move(notes))
	, m_profileSignature(std::move(profileSignature)) {}

DesignMetadata DesignMetadata::createNew(QString name,
										QString author,
										QString profileSignature,
										QString notes) {
	return DesignMetadata(std::move(name),
						  std::move(author),
						  QDateTime::currentDateTimeUtc(),
						  std::move(notes),
						  std::move(profileSignature));
}

void DesignMetadata::setCreatedUtc(QDateTime v) {
	m_createdUtc = normalizeUtc(std::move(v));
}

bool DesignMetadata::isValid() const noexcept {
	return m_createdUtc.isValid() && (m_createdUtc.timeSpec() == Qt::UTC);
}

} // namespace DesignModel