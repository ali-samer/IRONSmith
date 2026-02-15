// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtWidgets/QWidget>

namespace Core {

class GlobalMenuBarWidget;
class PlaygroundWidget;

class FrameWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit FrameWidget(QWidget* parent = nullptr);

	GlobalMenuBarWidget* menuBarWidget() const;
	QWidget* ribbonHost() const;
	PlaygroundWidget* playground() const;

private:
	GlobalMenuBarWidget* m_menuBarWidget = nullptr;
	QWidget* m_ribbonHost = nullptr;
	PlaygroundWidget* m_playground = nullptr;
};

} // namespace Core
