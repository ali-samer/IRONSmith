#pragma once

#include "designmodel/DesignModelGlobal.hpp"

#include <QtCore/QString>
#include <QtCore/QDateTime>

#include <optional>

namespace DesignModel {

class DESIGNMODEL_EXPORT DesignMetadata final {
public:
	DesignMetadata() = default;

	DesignMetadata(QString name,
				   QString author,
				   QDateTime createdUtc,
				   QString notes = {},
				   QString profileSignature = {});

	static DesignMetadata createNew(QString name,
									QString author,
									QString profileSignature = {},
									QString notes = {});

	const QString& name() const noexcept { return m_name; }
	const QString& author() const noexcept { return m_author; }
	const QDateTime& createdUtc() const noexcept { return m_createdUtc; }
	const QString& notes() const noexcept { return m_notes; }

	const QString& profileSignature() const noexcept { return m_profileSignature; }

	void setName(QString v) { m_name = std::move(v); }
	void setAuthor(QString v) { m_author = std::move(v); }
	void setCreatedUtc(QDateTime v);
	void setNotes(QString v) { m_notes = std::move(v); }
	void setProfileSignature(QString v) { m_profileSignature = std::move(v); }

	bool isValid() const noexcept;

private:
	static QDateTime normalizeUtc(QDateTime dt);

	QString m_name;
	QString m_author;
	QDateTime m_createdUtc; // always UTC if valid
	QString m_notes;
	QString m_profileSignature;
};

} // namespace DesignModel
