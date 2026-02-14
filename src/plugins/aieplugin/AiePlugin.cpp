#include "aieplugin/AieGlobal.hpp"
#include "aieplugin/AieConstants.hpp"
#include "aieplugin/NpuProfileLoader.hpp"
#include "aieplugin/design/CanvasDocumentImporter.hpp"
#include "aieplugin/design/DesignBundleLoader.hpp"
#include "aieplugin/design/DesignOpenController.hpp"
#include "aieplugin/design/DesignPersistenceController.hpp"
#include "aieplugin/panels/AieNewDesignDialog.hpp"
#include "aieplugin/panels/AieToolPanel.hpp"
#include "aieplugin/AieService.hpp"
#include "aieplugin/state/AiePanelState.hpp"

#include <extensionsystem/IPlugin.hpp>
#include <utils/Result.hpp>

#include <QtWidgets/QApplication>
#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>
#include <memory>

#include "core/api/ISidebarRegistry.hpp"
#include "core/api/SidebarToolSpec.hpp"
#include "core/api/IHeaderInfo.hpp"
#include "core/CoreConstants.hpp"
#include "core/ui/IUiHost.hpp"
#include "canvas/api/ICanvasHost.hpp"
#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/api/ICanvasStyleHost.hpp"
#include "canvas/api/CanvasStyleTypes.hpp"
#include "extensionsystem/PluginManager.hpp"
#include "projectexplorer/api/IProjectExplorer.hpp"

Q_LOGGING_CATEGORY(aiepluginlog, "ironsmith.aie")

extern int qInitResources_AiePluginResources();

namespace Aie::Internal {

class AiePlugin final : public ExtensionSystem::IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "AiePlugin.json")

public:
    AiePlugin() = default;

    Utils::Result initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager) override;
    void extensionsInitialized(ExtensionSystem::PluginManager& manager) override;
    ShutdownFlag aboutToShutdown() override;

private:
    void connectRibbonActions(Core::IUiHost* uiHost, ExtensionSystem::PluginManager& manager);

    QPointer<Core::ISidebarRegistry> m_sidebarRegistry;
    QPointer<AieService> m_service;
    QPointer<AiePanelState> m_panelState;
    QPointer<DesignOpenController> m_designOpenController;
    QPointer<DesignPersistenceController> m_persistenceController;
    std::unique_ptr<DesignBundleLoader> m_bundleLoader;
    std::unique_ptr<CanvasDocumentImporter> m_canvasImporter;
    bool m_toolRegistered = false;
};

namespace {

void logResultErrors(const QString& context, const Utils::Result& result)
{
    if (result.ok)
        return;
    qCWarning(aiepluginlog).noquote() << context;
    for (const auto& error : result.errors)
        qCWarning(aiepluginlog).noquote() << "  " << error;
}

QString formatDeviceLabel(const Aie::NpuProfileCatalog& catalog, const QString& deviceId)
{
    const auto* profile = findProfileById(catalog, deviceId);
    if (!profile) {
        if (deviceId.isEmpty())
            return QStringLiteral("PHOENIX-XDNA1");
        return deviceId.toUpper();
    }

    const QString name = profile->name.trimmed().toUpper();
    const QString family = profile->family.trimmed().toUpper();
    if (!name.isEmpty() && !family.isEmpty())
        return QStringLiteral("%1-%2").arg(name, family);
    if (!name.isEmpty())
        return name;
    if (!family.isEmpty())
        return family;
    return profile->id.trimmed().toUpper();
}

} // namespace

Utils::Result AiePlugin::initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager)
{
    Q_UNUSED(arguments);
    qInitResources_AiePluginResources();
    m_service = new AieService(this);
    ExtensionSystem::PluginManager::addObject(m_service);
    return Utils::Result::success();
}

void AiePlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
    auto* uiHost = manager.getObject<Core::IUiHost>();
    if (!uiHost) {
        qCWarning(aiepluginlog) << "AiePlugin: IUiHost not available.";
        return;
    }

    auto* gridHost = manager.getObject<Canvas::Api::ICanvasGridHost>();
    if (!gridHost) {
        qCWarning(aiepluginlog) << "AiePlugin: Canvas grid host not available.";
        return;
    }
    auto* canvasHost = manager.getObject<Canvas::Api::ICanvasHost>();
    auto* styleHost = manager.getObject<Canvas::Api::ICanvasStyleHost>();

    if (!m_service) {
        qCWarning(aiepluginlog) << "AiePlugin: AieService not available.";
        return;
    }
    m_service->setGridHost(gridHost);
    m_service->setStyleHost(styleHost);
    m_service->setCanvasHost(canvasHost);

    const Utils::Result loadResult =
        m_service->loadProfileCatalog(QString::fromLatin1(Aie::kDeviceTopologiesResource));
    if (!loadResult) {
        logResultErrors(QStringLiteral("AiePlugin: Failed to load NPU profile catalog."), loadResult);
        return;
    }

    QHash<QString, Canvas::Api::CanvasBlockStyle> baseStyles;
    Canvas::Api::CanvasBlockStyle aieStyle;
    aieStyle.fillColor = QColor(QStringLiteral("#0B1B16"));
    aieStyle.outlineColor = QColor(QStringLiteral("#16E08C"));
    aieStyle.labelColor = QColor(QStringLiteral("#B7F3DA"));
    aieStyle.cornerRadius = 0.0;
    baseStyles.insert(QStringLiteral("aie"), aieStyle);

    Canvas::Api::CanvasBlockStyle memStyle;
    memStyle.fillColor = QColor(QStringLiteral("#1F1710"));
    memStyle.outlineColor = QColor(QStringLiteral("#F2A14C"));
    memStyle.labelColor = QColor(QStringLiteral("#FFD7A8"));
    memStyle.cornerRadius = 0.0;
    baseStyles.insert(QStringLiteral("mem"), memStyle);

    Canvas::Api::CanvasBlockStyle shimStyle;
    shimStyle.fillColor = QColor(QStringLiteral("#0E1722"));
    shimStyle.outlineColor = QColor(QStringLiteral("#58B5FF"));
    shimStyle.labelColor = QColor(QStringLiteral("#CFE9FF"));
    shimStyle.cornerRadius = 0.0;
    baseStyles.insert(QStringLiteral("shim"), shimStyle);

    Canvas::Api::CanvasBlockStyle ddrStyle;
    ddrStyle.fillColor = QColor(QStringLiteral("#0F1116"));
    ddrStyle.outlineColor = QColor(QStringLiteral("#E6E9EF"));
    ddrStyle.labelColor = QColor(QStringLiteral("#E6E9EF"));
    ddrStyle.cornerRadius = 0.0;
    baseStyles.insert(QStringLiteral("ddr"), ddrStyle);

    m_service->setBaseStyles(baseStyles);

    if (canvasHost) {
        canvasHost->setEmptyStateText(QStringLiteral("No design open."),
                                      QStringLiteral("Create or open a design to start."));
        canvasHost->setCanvasActive(false);
    }

    if (!m_bundleLoader)
        m_bundleLoader = std::make_unique<DesignBundleLoader>(&m_service->catalog());
    if (!m_canvasImporter)
        m_canvasImporter = std::make_unique<CanvasDocumentImporter>(m_service);
    if (!m_persistenceController)
        m_persistenceController = new DesignPersistenceController(this);
    if (m_persistenceController)
        m_persistenceController->setCanvasHost(canvasHost);
    if (m_persistenceController && m_service)
        m_persistenceController->setCoordinator(m_service->coordinator());
    if (!m_designOpenController) {
        m_designOpenController = new DesignOpenController(m_bundleLoader.get(),
                                                          m_canvasImporter.get(),
                                                          m_persistenceController,
                                                          this);
    }
    if (m_designOpenController) {
        m_designOpenController->setUiHost(uiHost);
        if (auto* explorer = manager.getObject<ProjectExplorer::IProjectExplorer>())
            m_designOpenController->setProjectExplorer(explorer);
    }
    if (m_designOpenController) {
        if (auto* header = manager.getObject<Core::IHeaderInfo>()) {
            connect(m_designOpenController, &DesignOpenController::designOpened,
                    this, [this, header](const QString&, const QString& displayName, const QString& deviceId) {
                        header->setDesignLabel(displayName);
                        if (m_service)
                            header->setDeviceLabel(formatDeviceLabel(m_service->catalog(), deviceId));
                        else
                            header->setDeviceLabel(deviceId);
                    });
        }
    }

    if (!m_panelState)
        m_panelState = new AiePanelState(m_service->coordinator(), this);
    else
        m_panelState->setCoordinator(m_service->coordinator());

    if (!m_sidebarRegistry)
        m_sidebarRegistry = uiHost->sidebarRegistry();

    if (m_sidebarRegistry && !m_toolRegistered) {
        Core::SidebarToolSpec spec;
        spec.id = QStringLiteral("IRONSmith.AieGridTools");
        spec.title = QStringLiteral("AIE");
        spec.iconResource = QStringLiteral(":/ui/icons/svg/hammer_icon.svg");
        spec.side = Core::SidebarSide::Right;
        spec.family = Core::SidebarFamily::Vertical;
        spec.region = Core::SidebarRegion::Exclusive;
        spec.rail = Core::SidebarRail::Top;
        spec.order = 0;
        spec.toolTip = QStringLiteral("AIE Grid Tools");

        const auto factory = [this](QWidget* parent) -> QWidget* {
            return new AieToolPanel(m_service ? m_service->coordinator() : nullptr, parent);
        };

        QString error;
        if (!m_sidebarRegistry->registerTool(spec, factory, &error)) {
            qCWarning(aiepluginlog) << "AiePlugin: registerTool failed:" << error;
        } else {
            m_toolRegistered = true;
        }
    }

    connectRibbonActions(uiHost, manager);
}

ExtensionSystem::IPlugin::ShutdownFlag AiePlugin::aboutToShutdown()
{
    if (m_sidebarRegistry && m_toolRegistered) {
        QString error;
        if (!m_sidebarRegistry->unregisterTool(QStringLiteral("IRONSmith.AieGridTools"), &error)) {
            qCWarning(aiepluginlog) << "AiePlugin: unregisterTool failed:" << error;
        }
    }
    if (m_service) {
        ExtensionSystem::PluginManager::removeObject(m_service);
        m_service = nullptr;
    }
    return ShutdownFlag::SynchronousShutdown;
}

void AiePlugin::connectRibbonActions(Core::IUiHost* uiHost, ExtensionSystem::PluginManager& manager)
{
    if (!uiHost)
        return;

    QAction* newDesignAction = uiHost->ribbonCommand(Core::Constants::RIBBON_TAB_HOME,
                                                     Core::Constants::RIBBON_TAB_HOME_PROJECT_GROUP,
                                                     Core::Constants::PROJECT_NEW_ITEMID);
    if (!newDesignAction) {
        qCWarning(aiepluginlog) << "AiePlugin: New Design action not available.";
        return;
    }

    auto* projectExplorer = manager.getObject<ProjectExplorer::IProjectExplorer>();

    connect(newDesignAction, &QAction::triggered, this, [this, uiHost, projectExplorer]() {
        QWidget* parent = uiHost ? uiHost->playgroundOverlayHost() : nullptr;
        if (!parent)
            parent = QApplication::activeWindow();

        AieNewDesignDialog dialog(parent);
        if (dialog.exec() != QDialog::Accepted)
            return;

        const auto result = dialog.result();
        if (!result.created)
            return;

        if (projectExplorer)
            projectExplorer->refresh();
        if (m_designOpenController)
            m_designOpenController->openBundlePath(result.bundlePath);
    });
}

} // namespace Aie::Internal

#include "AiePlugin.moc"
