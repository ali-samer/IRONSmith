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

	QWidget* m_baseHost = nullptr;
	QWidget* m_chromeOverlay = nullptr;

	QStackedLayout* m_rootStack = nullptr;
};

} // namespace Core