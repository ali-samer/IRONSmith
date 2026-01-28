#pragma once

#include <QtWidgets/QWidget>

class QLabel;

namespace Core {

class StatusBarField;

class StatusBarFieldWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit StatusBarFieldWidget(StatusBarField* field, QWidget* parent = nullptr);
	~StatusBarFieldWidget() override = default;

	StatusBarField* field() const noexcept { return m_field; }

private slots:
	void syncFromModel();

private:
	StatusBarField* m_field = nullptr;
	QLabel* m_label = nullptr;
	QLabel* m_value = nullptr;
};

} // namespace Core