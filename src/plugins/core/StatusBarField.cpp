#include "core/StatusBarField.hpp"

namespace Core {

static bool isNonEmptyId(const QString& id)
{
	return !id.trimmed().isEmpty();
}

StatusBarField::StatusBarField(QString id, QObject* parent)
	: QObject(parent)
	, m_id(std::move(id))
{
	Q_ASSERT(isNonEmptyId(m_id));
}

void StatusBarField::setLabel(QString label)
{
	if (m_label == label)
		return;

	m_label = std::move(label);
	emit labelChanged(m_label);
	emit changed();
}

void StatusBarField::setValue(QString value)
{
	if (m_value == value)
		return;

	m_value = std::move(value);
	emit valueChanged(m_value);
	emit changed();
}

void StatusBarField::setSide(Side side)
{
	if (m_side == side)
		return;

	m_side = side;
	emit sideChanged(m_side);
	emit changed();
}

} // namespace Core