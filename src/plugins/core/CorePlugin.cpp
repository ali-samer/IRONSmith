#include "core/CorePlugin.hpp"

#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QDebug>

#include <extensionsystem/PluginManager.hpp>

#include "core/CoreGlobal.hpp"
#include "core/CoreImpl.hpp"
#include "core/CoreConstants.hpp"
#include "core/ui/IUiHost.hpp"

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

	if (auto* ui = m_core->uiHost())
		ExtensionSystem::PluginManager::removeObject(ui);

}

Utils::Result CorePlugin::initialize(const QStringList& arguments,
									 ExtensionSystem::PluginManager& manager)
{
	Q_UNUSED(arguments);
	Q_UNUSED(manager);

	if (m_core)
		return Utils::Result::failure("CorePlugin initialized twice.");

	m_core = new CoreImpl(this);

	auto* ui = m_core->uiHost();
	if (!ui)
		return Utils::Result::failure("CorePlugin failed to create IUiHost.");

	ExtensionSystem::PluginManager::addObject(ui);

	ui->addMenuTab(Constants::RIBBON_TAB_HOME , "Home");
	ui->addMenuTab(Constants::RIBBON_TAB_VIEW, "View");
	ui->addMenuTab(Constants::RIBBON_TAB_OUTPUT, "Output");

	ui->setActiveMenuTab(Constants::RIBBON_TAB_HOME);
	ui->ensureRibbonGroup(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "Project");

	auto* actNew    = new QAction(tr("New Design"), this);
	auto* actOpen   = new QAction(tr("Open…"), this);
	auto* actSave   = new QAction(tr("Save"), this);
	auto* actSaveAs = new QAction(tr("Save As…"), this);

	// actNew->setIcon(QIcon(":/ui/icons/32x32/document-new.png"));
	actNew->setIcon(QIcon(":/ui/icons/64x64/file-new.png"));
	actOpen->setIcon(QIcon(":/ui/icons/64x64/folder-new.png"));
	actSave->setIcon(QIcon(":/ui/icons/64x64/file-save.png"));
	actSaveAs->setIcon(QIcon(":/ui/icons/64x64/file-save-as.png"));

	auto* recentMenu = new QMenu;
	connect(this, &QObject::destroyed, recentMenu, &QObject::deleteLater);
	{
		auto* placeholder = recentMenu->addAction(tr("No recent projects"));
		placeholder->setEnabled(false);
	}

	auto* actRecent = new QAction(tr("Recent"), this);
	actRecent->setMenu(recentMenu);
	actRecent->setIcon(QIcon(":/ui/icons/32x32/document-open-recent.png"));

	connect(actNew, &QAction::triggered, this, [] { qCWarning(corelog) << "New Design triggered (no project service bound yet)."; });
	connect(actOpen, &QAction::triggered, this, [] { qCWarning(corelog) << "Open triggered (no project service bound yet)."; });
	connect(actSave, &QAction::triggered, this, [] { qCWarning(corelog) << "Save triggered (no project service bound yet)."; });
	connect(actSaveAs, &QAction::triggered, this, [] { qCWarning(corelog) << "Save As triggered (no project service bound yet)."; });

	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, Constants::PROJECT_NEW_ITEMID,    actNew,    RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, Constants::PROJECT_OPEN_ITEMID,   actOpen,   RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, Constants::PROJECT_SAVE_ITEMID,   actSave,   RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, Constants::PROJECT_SAVE_AS_ITEMID, actSaveAs, RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, Constants::PROJECT_RECENT_ITEMID, actRecent, RibbonControlType::DropDownButton, {});

	return Utils::Result::success();
}

void CorePlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
	Q_UNUSED(manager);

	if (m_core)
		m_core->open();
}

} // namespace Core
