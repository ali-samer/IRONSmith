#include "core/widgets/StatusBarFieldWidget.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QStyle>

#include "core/StatusBarField.hpp"

namespace Core {

StatusBarFieldWidget::StatusBarFieldWidget(StatusBarField* field, QWidget* parent)
	: QWidget(parent)
	, m_field(field)
{
	Q_ASSERT(m_field);

	setObjectName("StatusBarField");
	setAttribute(Qt::WA_StyledBackground, true);

	m_row = new QHBoxLayout(this);
	m_row->setContentsMargins(0, 0, 0, 0);
	m_row->setSpacing(6);

	m_label = new QLabel(this);
	m_label->setObjectName("StatusBarFieldLabel");
	m_label->setTextFormat(Qt::PlainText);
	m_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	m_value = new QLabel(this);
	m_value->setObjectName("StatusBarFieldValue");
	m_value->setTextFormat(Qt::PlainText);
	m_value->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	m_value->setAttribute(Qt::WA_StyledBackground, true);

	m_modeBadge = new QLabel(this);
	m_modeBadge->setObjectName("StatusBarFieldModeBadge");
	m_modeBadge->setTextFormat(Qt::PlainText);
	m_modeBadge->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	m_modeBadge->setAttribute(Qt::WA_StyledBackground, true);
	m_modeBadge->setVisible(false);

	m_row->addWidget(m_label, 0);
	m_row->addWidget(m_value, 0);
	m_row->addWidget(m_modeBadge, 0);

	connect(m_field, &StatusBarField::changed, this, &StatusBarFieldWidget::syncFromModel);

	syncFromModel();
}

void StatusBarFieldWidget::syncFromModel()
{
	if (!m_field)
		return;

	const bool isModeField = (m_field->id() == QStringLiteral("mode"));
	const QString rawValue = m_field->value().trimmed();
	QString modeValue = isModeField ? rawValue.toUpper() : QString();
	QString modeBadge;
	if (isModeField && rawValue.contains('|')) {
		const QStringList parts = rawValue.split('|');
		if (!parts.isEmpty())
			modeValue = parts.value(0).trimmed().toUpper();
		if (parts.size() > 1)
			modeBadge = parts.value(1).trimmed().toUpper();
	}

	const QString lbl = m_field->label().isEmpty() ? QString() : (m_field->label() + ":");
	m_label->setText(lbl);
	m_value->setText(isModeField ? modeValue : m_field->value());

	m_label->setVisible(!isModeField);
	m_value->setSizePolicy(isModeField ? QSizePolicy::Minimum : QSizePolicy::MinimumExpanding,
	                       QSizePolicy::Preferred);
	if (m_row)
		m_row->setSpacing(isModeField ? 0 : 6);

	const QVariant newModeProp = isModeField ? QVariant(modeValue) : QVariant();
	if (m_value->property("mode") != newModeProp) {
		m_value->setProperty("mode", newModeProp);
		if (auto* s = m_value->style()) {
			s->unpolish(m_value);
			s->polish(m_value);
		}
		m_value->update();
	}

	if (isModeField && !modeBadge.isEmpty()) {
		m_modeBadge->setVisible(true);
		m_modeBadge->setText(modeBadge);
		const QVariant newLinkProp = QVariant(modeBadge);
		if (m_modeBadge->property("linkmode") != newLinkProp) {
			m_modeBadge->setProperty("linkmode", newLinkProp);
			if (auto* s = m_modeBadge->style()) {
				s->unpolish(m_modeBadge);
				s->polish(m_modeBadge);
			}
			m_modeBadge->update();
		}
		const QVariant newSubProp = QVariant(QStringLiteral("1"));
		if (m_value->property("linksub") != newSubProp) {
			m_value->setProperty("linksub", newSubProp);
			if (auto* s = m_value->style()) {
				s->unpolish(m_value);
				s->polish(m_value);
			}
			m_value->update();
		}
	} else {
		m_modeBadge->setVisible(false);
		if (m_value->property("linksub").isValid()) {
			m_value->setProperty("linksub", QVariant());
			if (auto* s = m_value->style()) {
				s->unpolish(m_value);
				s->polish(m_value);
			}
			m_value->update();
		}
	}
}

} // namespace Core
