// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtWidgets/QWidget>

class QLabel;
class QHBoxLayout;

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
	QHBoxLayout* m_row = nullptr;
	QLabel* m_label = nullptr;
	QLabel* m_value = nullptr;
	QLabel* m_modeBadge = nullptr;
};

} // namespace Core
