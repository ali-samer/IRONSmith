#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include "core/CoreGlobal.hpp"

namespace Core {

class CORE_EXPORT StatusBarField final : public QObject
{
	Q_OBJECT

public:
	enum class Side { Left, Right };
	Q_ENUM(Side)

	explicit StatusBarField(QString id, QObject* parent = nullptr);
	~StatusBarField() override = default;

	const QString& id() const noexcept { return m_id; }

	const QString& label() const noexcept { return m_label; }
	const QString& value() const noexcept { return m_value; }

	Side side() const noexcept { return m_side; }

public slots:
	void setLabel(QString label);
	void setValue(QString value);
	void setSide(Side side);

signals:
	void labelChanged(const QString& label);
	void valueChanged(const QString& value);
	void sideChanged(Side side);

	void changed();

private:
	QString m_id;
	QString m_label;
	QString m_value;
	Side m_side = Side::Left;
};

} // namespace Core
