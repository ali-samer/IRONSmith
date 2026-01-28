#include "core/widgets/StatusBarFieldWidget.hpp"

#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

#include "core/StatusBarField.hpp"

namespace Core {

StatusBarFieldWidget::StatusBarFieldWidget(StatusBarField* field, QWidget* parent)
	: QWidget(parent)
	, m_field(field)
{
	Q_ASSERT(m_field);

	setObjectName("StatusBarField");
	setAttribute(Qt::WA_StyledBackground, true);

	auto* row = new QHBoxLayout(this);
	row->setContentsMargins(0, 0, 0, 0);
	row->setSpacing(6);

	m_label = new QLabel(this);
	m_label->setObjectName("StatusBarFieldLabel");
	m_label->setTextFormat(Qt::PlainText);
	m_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	m_value = new QLabel(this);
	m_value->setObjectName("StatusBarFieldValue");
	m_value->setTextFormat(Qt::PlainText);
	m_value->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	row->addWidget(m_label, 0);
	row->addWidget(m_value, 0);

	connect(m_field, &StatusBarField::changed, this, &StatusBarFieldWidget::syncFromModel);

	syncFromModel();
}

void StatusBarFieldWidget::syncFromModel()
{
	if (!m_field)
		return;

	const QString lbl = m_field->label().isEmpty() ? QString() : (m_field->label() + ":");
	m_label->setText(lbl);
	m_value->setText(m_field->value());
}

} // namespace Core