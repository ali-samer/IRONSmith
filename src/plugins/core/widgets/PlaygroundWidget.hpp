// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtWidgets/QWidget>

class QStackedLayout;

namespace Core {

class InfoBarWidget;

class PlaygroundWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit PlaygroundWidget(QWidget* parent = nullptr);

	InfoBarWidget* topBar() const;
	InfoBarWidget* bottomBar() const;

	QWidget* leftSidebarHost() const;
	QWidget* rightSidebarHost() const;

	QWidget* leftSidebarPanelHost() const;
	QWidget* rightSidebarPanelHost() const;

	QWidget* centerBaseHost() const;

	QWidget* overlayHost() const;

private:
	InfoBarWidget* m_topBar = nullptr;
	InfoBarWidget* m_bottomBar = nullptr;

	QWidget* m_leftSidebarContainer = nullptr;
	QWidget* m_rightSidebarContainer = nullptr;

	QWidget* m_leftSidebarInstallHost = nullptr;
	QWidget* m_rightSidebarInstallHost = nullptr;

	QWidget* m_leftSidebarPanelInstallHost = nullptr;
	QWidget* m_rightSidebarPanelInstallHost = nullptr;

	// Center "content rect" is a stacked container:
	//  - base: the primary editor surface (canvas)
	//  - overlay: tool panels that render on top without resizing the base
	QWidget* m_centerContainer = nullptr;
	QWidget* m_baseHost = nullptr;
	QWidget* m_centerOverlay = nullptr;
	QStackedLayout* m_centerStack = nullptr;
};

} // namespace Core
