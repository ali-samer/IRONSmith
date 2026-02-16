// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "core/CorePlugin.hpp"

#include <QtGui/QAction>
#include <QActionGroup>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QDebug>
#include <QElapsedTimer>

#include <extensionsystem/PluginManager.hpp>

#include "core/CoreGlobal.hpp"
#include "core/CoreImpl.hpp"
#include "core/CoreConstants.hpp"
#include "core/CommandRibbon.hpp"
#include "core/HeaderInfoService.hpp"
#include "core/ui/IUiHost.hpp"
#include "core/widgets/InfoBarWidget.hpp"

// Sidebar test wiring (Core-internal)
#include "core/SidebarRegistryImpl.hpp"
#include "core/widgets/ToolRailWidget.hpp"
#include "core/ui/UiStyle.hpp"

namespace Core {

CorePlugin::CorePlugin(QObject* parent)
	: ExtensionSystem::IPlugin(parent)
{
}

CorePlugin::~CorePlugin()
{
	if (!m_core)
		return;

	if (m_headerInfo)
		ExtensionSystem::PluginManager::removeObject(static_cast<Core::IHeaderInfo*>(m_headerInfo));

	if (auto* ui = m_core->uiHost())
		ExtensionSystem::PluginManager::removeObject(ui);

}

Utils::Result CorePlugin::initialize(const QStringList& arguments,
									 ExtensionSystem::PluginManager& manager)
{
	Q_UNUSED(arguments);
	Q_UNUSED(manager);

	qCInfo(corelog) << "Initializing...";
	if (m_core)
		return Utils::Result::failure("CorePlugin initialized twice.");

	m_core = new CoreImpl(this);

	auto* ui = m_core->uiHost();
	if (!ui)
		return Utils::Result::failure("CorePlugin failed to create IUiHost.");

	ExtensionSystem::PluginManager::addObject(ui);

	qCInfo(corelog) << "setting up command ribbon actions.";
	QElapsedTimer timer;
	timer.start();
	setupCommandRibbonActions(ui);
	qint64 ns = timer.nsecsElapsed();
	qCInfo(corelog) << "command ribbon actions setup complete: avg =" << (ns/1e9) << " secs.";
	if (auto* bar = ui->playgroundTopBar()) {
		m_headerInfo = new Internal::HeaderInfoService(this);
		m_headerInfo->bindInfoBar(bar);
		ExtensionSystem::PluginManager::addObject(static_cast<Core::IHeaderInfo*>(m_headerInfo));
	}

	return Utils::Result::success();
}

void CorePlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
	Q_UNUSED(manager);

	qCInfo(corelog) << "extensionsInitialized()...";
	if (m_core)
		m_core->open();
}

void CorePlugin::setupCommandRibbonActions(Core::IUiHost* uiHost)
{
	if (!uiHost)
		return;

	struct RibbonUpdateBatchGuard final
	{
		Core::IUiHost* host = nullptr;
		~RibbonUpdateBatchGuard()
		{
			if (host)
				host->endRibbonUpdateBatch();
		}
	};

	uiHost->beginRibbonUpdateBatch();
	const RibbonUpdateBatchGuard batchGuard{uiHost};

	uiHost->addMenuTab(Constants::RIBBON_TAB_HOME, "Home");
	uiHost->addMenuTab(Constants::RIBBON_TAB_VIEW, "View");
	uiHost->addMenuTab(Constants::RIBBON_TAB_OUTPUT, "Output");

	uiHost->setActiveMenuTab(Constants::RIBBON_TAB_HOME);

	setupHomePageCommands(uiHost);
	setupViewPageCommands(uiHost);
	setupOutputPageCommands(uiHost);
}


void CorePlugin::setupHomePageCommands(Core::IUiHost* uiHost)
{
	if (!uiHost)
		return;

	uiHost->ensureRibbonGroup(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "Project");
	uiHost->ensureRibbonGroup(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_CANVAS_GROUP, "Canvas");
	uiHost->ensureRibbonGroup(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_WIRES_GROUP, "Wires");
	uiHost->ensureRibbonGroup(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_VIEW_GROUP, "View");

	auto* actNew    = new QAction(tr("New Design"), this);
	auto* actOpen   = new QAction(tr("Open…"), this);

	// actNew->setIcon(QIcon(":/ui/icons/32x32/document-new.png"));
	actNew->setIcon(QIcon( ":/ui/icons/svg/file_new_icon.svg"));
	actOpen->setIcon(QIcon(":/ui/icons/svg/opened_folder.svg"));

	connect(actNew, &QAction::triggered, this, [] {
		qCWarning(corelog) << "New Design triggered (no project service bound yet).";
	});

	auto projectRoot = RibbonNode::makeRow("project_root");
	projectRoot->addCommand(Constants::PROJECT_NEW_ITEMID, actNew, RibbonControlType::Button, {});
	projectRoot->addCommand(Constants::PROJECT_OPEN_ITEMID, actOpen, RibbonControlType::Button, {});

	if (const auto res = uiHost->setRibbonGroupLayout(Constants::RIBBON_TAB_HOME,
	                                                  Constants::RIBBON_TAB_HOME_PROJECT_GROUP,
	                                                  std::move(projectRoot));
	    !res) {
		qCWarning(corelog) << res.error;
	}

	auto* actSelect = new QAction(tr("Select"), this);
	auto* actPan = new QAction(tr("Pan"), this);
	auto* actLink = new QAction(tr("Link"), this);
	auto* actSplit = new QAction(tr("Split"), this);
	auto* actJoin = new QAction(tr("Join"), this);
	auto* actBroadcast = new QAction(tr("Broadcast"), this);

	actSelect->setCheckable(true);
	actPan->setCheckable(true);
	actLink->setCheckable(true);
	actSplit->setCheckable(true);
	actJoin->setCheckable(true);
	actBroadcast->setCheckable(true);

	actSelect->setIcon(QIcon(":/ui/icons/svg/select_hand_pointer_icon.svg"));
	actPan->setIcon(QIcon(":/ui/icons/svg/pan_icon.svg"));
	actLink->setIcon(QIcon(":/ui/icons/svg/link_icon.svg"));
	actSplit->setIcon(QIcon(":/ui/icons/svg/split_link_icon.svg"));
	actJoin->setIcon(QIcon(":/ui/icons/svg/merge_link_icon.svg"));
	actBroadcast->setIcon(QIcon(":/ui/icons/svg/broadcast_link_icon.svg"));

	RibbonPresentation smallPres;
	smallPres.size = RibbonVisualSize::Small;
	smallPres.iconPx = 20;

	RibbonPresentation linkPres = smallPres;
	linkPres.size = RibbonVisualSize::Large;
	linkPres.iconPx = 32;

	auto canvasRoot = RibbonNode::makeRow("canvas_root");
	canvasRoot->addCommand(Constants::CANVAS_SELECT_ITEMID, actSelect, RibbonControlType::ToggleButton, smallPres);
	canvasRoot->addCommand(Constants::CANVAS_PAN_ITEMID, actPan, RibbonControlType::ToggleButton, smallPres);
	canvasRoot->addSeparator("canvas_link_sep");

	canvasRoot->addCommand(Constants::CANVAS_LINK_ITEMID, actLink, RibbonControlType::ToggleButton, linkPres);
	canvasRoot->addCommand(Constants::CANVAS_LINK_SPLIT_ITEMID, actSplit, RibbonControlType::ToggleButton, smallPres);
	canvasRoot->addCommand(Constants::CANVAS_LINK_JOIN_ITEMID, actJoin, RibbonControlType::ToggleButton, smallPres);
	canvasRoot->addCommand(Constants::CANVAS_LINK_BROADCAST_ITEMID, actBroadcast, RibbonControlType::ToggleButton, smallPres);

	if (const auto res = uiHost->setRibbonGroupLayout(Constants::RIBBON_TAB_HOME,
	                                                  Constants::RIBBON_TAB_HOME_CANVAS_GROUP,
	                                                  std::move(canvasRoot));
	    !res) {
		qCWarning(corelog) << res.error;
	}

	auto* actAutoRoute = new QAction(tr("Auto Route"), this);
	auto* actClearOverrides = new QAction(tr("Clear Overrides"), this);
	auto* actToggleArrows = new QAction(tr("Wire Arrows"), this);

	actToggleArrows->setCheckable(true);
	actAutoRoute->setIcon(QIcon(":/ui/icons/svg/auto_route_icon.svg"));

	auto wiresRoot = RibbonNode::makeRow("wires_root");
	wiresRoot->addCommand(Constants::CANVAS_WIRE_AUTO_ROUTE_ITEMID, actAutoRoute, RibbonControlType::Button, smallPres);
	wiresRoot->addCommand(Constants::CANVAS_WIRE_CLEAR_OVERRIDES_ITEMID, actClearOverrides, RibbonControlType::Button, smallPres);
	wiresRoot->addCommand(Constants::CANVAS_WIRE_TOGGLE_ARROWS_ITEMID, actToggleArrows, RibbonControlType::ToggleButton, smallPres);

	if (const auto res = uiHost->setRibbonGroupLayout(Constants::RIBBON_TAB_HOME,
	                                                  Constants::RIBBON_TAB_HOME_WIRES_GROUP,
	                                                  std::move(wiresRoot));
	    !res) {
		qCWarning(corelog) << res.error;
	}
	auto* actZoomIn = new QAction(tr("Zoom In"), this);
	auto* actZoomOut = new QAction(tr("Zoom Out"), this);
	auto* actZoomFit = new QAction(tr("Zoom to Fit"), this);
	auto* actResetView = new QAction(tr("Reset View"), this);

	actZoomIn->setIcon(QIcon(":/ui/icons/svg/zoom_in_icon.svg"));
	actZoomOut->setIcon(QIcon(":/ui/icons/svg/zoom_out_icon.svg"));
	actZoomFit->setIcon(QIcon(":/ui/icons/svg/zoom_fit_icon.svg"));
	actResetView->setIcon(QIcon(":/ui/icons/svg/reset_icon.svg"));

	auto viewRoot = RibbonNode::makeRow("view_root");
	viewRoot->addCommand(Constants::CANVAS_VIEW_ZOOM_IN_ITEMID, actZoomIn, RibbonControlType::Button, smallPres);
	viewRoot->addCommand(Constants::CANVAS_VIEW_ZOOM_OUT_ITEMID, actZoomOut, RibbonControlType::Button, smallPres);
	viewRoot->addCommand(Constants::CANVAS_VIEW_ZOOM_FIT_ITEMID, actZoomFit, RibbonControlType::Button, smallPres);
	viewRoot->addCommand(Constants::CANVAS_VIEW_RESET_ITEMID, actResetView, RibbonControlType::Button, smallPres);

	if (const auto res = uiHost->setRibbonGroupLayout(Constants::RIBBON_TAB_HOME,
	                                                  Constants::RIBBON_TAB_HOME_VIEW_GROUP,
	                                                  std::move(viewRoot));
	    !res) {
		qCWarning(corelog) << res.error;
	}
}

void CorePlugin::setupViewPageCommands(Core::IUiHost* uiHost)
{
	Q_UNUSED(uiHost);
}

void CorePlugin::setupOutputPageCommands(Core::IUiHost* uiHost)
{
	Q_UNUSED(uiHost);
}

} // namespace Core
