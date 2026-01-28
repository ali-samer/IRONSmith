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

	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "project.new",    actNew,    RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "project.open",   actOpen,   RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "project.save",   actSave,   RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "project.saveas", actSaveAs, RibbonControlType::Button,         {});
	ui->addRibbonCommand(Constants::RIBBON_TAB_HOME, Constants::RIBBON_TAB_HOME_PROJECT_GROUP, "project.recent", actRecent, RibbonControlType::DropDownButton, {});

	// TEMP: Dummy sidebar panels for validating rail grouping, family routing, and region slot behavior.
	if (auto* sidebar = ui->sidebarRegistry()) {
		auto registerDummy = [sidebar](const QString& id,
		                              SidebarSide side,
		                              SidebarFamily family,
		                              SidebarRegion region,
		                              const QString& title) {
			SidebarToolSpec spec;
			spec.id = id;
			spec.side = side;
			spec.family = family;
			spec.region = region;
			spec.title = title;
			spec.toolTip = title;
			spec.iconResource = ":/ui/icons/128x128/folder-yellow.png";

			sidebar->registerTool(spec, [title, id](QWidget* parent) -> QWidget* {
				auto* panel = new QFrame(parent);
				panel->setObjectName(QString("DummySidebarPanel_%1").arg(id).replace('.', '_'));
				panel->setAttribute(Qt::WA_StyledBackground, true);

				auto* layout = new QVBoxLayout(panel);
				layout->setContentsMargins(12, 12, 12, 12);
				layout->setSpacing(8);

				auto* label = new QLabel(title, panel);
				label->setObjectName(QString("DummySidebarPanelTitle_%1").arg(id).replace('.', '_'));
				layout->addWidget(label, 0);
				layout->addStretch(1);
				return panel;
			});
		};

		registerDummy("core.dummy.v.el1", SidebarSide::Left,  SidebarFamily::Vertical,   SidebarRegion::Exclusive, "EL1");
		registerDummy("core.dummy.v.el2", SidebarSide::Left,  SidebarFamily::Vertical,   SidebarRegion::Exclusive, "EL2");
		registerDummy("core.dummy.v.al1", SidebarSide::Left,  SidebarFamily::Vertical,   SidebarRegion::Additive,  "AL1");
		registerDummy("core.dummy.v.al2", SidebarSide::Left,  SidebarFamily::Vertical,   SidebarRegion::Additive,  "AL2");
		registerDummy("core.dummy.v.er1", SidebarSide::Right, SidebarFamily::Vertical,   SidebarRegion::Exclusive, "ER1");
		registerDummy("core.dummy.v.er2", SidebarSide::Right, SidebarFamily::Vertical,   SidebarRegion::Exclusive, "ER2");
		registerDummy("core.dummy.v.ar1", SidebarSide::Right, SidebarFamily::Vertical,   SidebarRegion::Additive,  "AR1");
		registerDummy("core.dummy.v.ar2", SidebarSide::Right, SidebarFamily::Vertical,   SidebarRegion::Additive,  "AR2");

		registerDummy("core.dummy.h.hel1", SidebarSide::Left,  SidebarFamily::Horizontal, SidebarRegion::Exclusive, "HEL1");
		registerDummy("core.dummy.h.hel2", SidebarSide::Left,  SidebarFamily::Horizontal, SidebarRegion::Exclusive, "HEL2");
		registerDummy("core.dummy.h.hal1", SidebarSide::Left,  SidebarFamily::Horizontal, SidebarRegion::Additive,  "HAL1");
		registerDummy("core.dummy.h.hal2", SidebarSide::Left,  SidebarFamily::Horizontal, SidebarRegion::Additive,  "HAL2");
		registerDummy("core.dummy.h.her1", SidebarSide::Right, SidebarFamily::Horizontal, SidebarRegion::Exclusive, "HER1");
		registerDummy("core.dummy.h.her2", SidebarSide::Right, SidebarFamily::Horizontal, SidebarRegion::Exclusive, "HER2");
		registerDummy("core.dummy.h.har1", SidebarSide::Right, SidebarFamily::Horizontal, SidebarRegion::Additive,  "HAR1");
		registerDummy("core.dummy.h.har2", SidebarSide::Right, SidebarFamily::Horizontal, SidebarRegion::Additive,  "HAR2");
	}


	return Utils::Result::success();
}

void CorePlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
	Q_UNUSED(manager);

	if (m_core)
		m_core->open();
}

} // namespace Core