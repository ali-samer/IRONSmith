#pragma once

#include "core/ui/IUiHost.hpp"

namespace Core {

class CommandRibbon;
class CommandRibbonWidget;
class FrameWidget;
class PlaygroundWidget;
class GlobalMenuBar;
class SidebarRegistryImpl;
class ISidebarRegistry;

class UiHostImpl final : public IUiHost
{
	Q_OBJECT

public:
	explicit UiHostImpl(FrameWidget* frame, QObject* parent = nullptr);

	bool addMenuTab(QString id, QString title) override;
	bool setActiveMenuTab(QString id) override;
	QString activeMenuTab() const override;

	ISidebarRegistry* sidebarRegistry() const override;

	bool ensureRibbonPage(QString pageId, QString title) override;
	bool ensureRibbonGroup(QString pageId, QString groupId, QString title) override;
	RibbonResult setRibbonGroupLayout(QString pageId,
									 QString groupId,
									 std::unique_ptr<RibbonNode> root) override;

	QAction* ribbonCommand(QString pageId, QString groupId, QString itemId) override;
	RibbonResult addRibbonCommand(QString pageId,
								 QString groupId,
								 QString itemId,
								 QAction* action,
								 RibbonControlType type,
								 RibbonPresentation pres) override;
	RibbonResult addRibbonSeparator(QString pageId, QString groupId, QString itemId) override;
	RibbonResult addRibbonStretch(QString pageId, QString groupId, QString itemId) override;

	void setLeftSidebar(QWidget* w) override;
	void setRightSidebar(QWidget* w) override;

	void setPlaygroundTopBar(QWidget* w) override;
	void setPlaygroundBottomBar(QWidget* w) override;
	InfoBarWidget* playgroundTopBar() const override;
	InfoBarWidget* playgroundBottomBar() const override;

	void setPlaygroundCenterBase(QWidget* w) override;
	QWidget* playgroundOverlayHost() const override;

private:
	static void replaceSingleChild(QWidget* host, QWidget* child);

private:
	FrameWidget* m_frame = nullptr;
	GlobalMenuBar* m_menuModel = nullptr;
	CommandRibbon* m_ribbonModel = nullptr;
	CommandRibbonWidget* m_ribbonWidget = nullptr;
	PlaygroundWidget* m_playground = nullptr;
	SidebarRegistryImpl* m_sidebarRegistry = nullptr;
};

} // namespace Core
