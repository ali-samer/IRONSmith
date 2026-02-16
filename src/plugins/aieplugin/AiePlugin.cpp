// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/AieGlobal.hpp"
#include "aieplugin/AieConstants.hpp"
#include "aieplugin/AieStyleCatalog.hpp"
#include "aieplugin/NpuProfileLoader.hpp"
#include "aieplugin/design/CanvasDocumentImporter.hpp"
#include "aieplugin/design/DesignBundleLoader.hpp"
#include "aieplugin/design/DesignOpenController.hpp"
#include "aieplugin/panels/AieNewDesignDialog.hpp"
#include "aieplugin/panels/AieToolPanel.hpp"
#include "aieplugin/AieService.hpp"
#include "aieplugin/state/AiePanelState.hpp"
#include "aieplugin/state/AieWorkspaceState.hpp"

#include <extensionsystem/IPlugin.hpp>
#include <utils/DocumentBundle.hpp>
#include <utils/PathUtils.hpp>
#include <utils/Result.hpp>

#include <QtCore/QFileInfo>
#include <QtCore/QLoggingCategory>
#include <QtCore/QMetaObject>
#include <QtCore/QStringList>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <memory>

#include "core/api/ISidebarRegistry.hpp"
#include "core/api/SidebarToolSpec.hpp"
#include "core/api/IHeaderInfo.hpp"
#include "core/CoreConstants.hpp"
#include "core/ui/IUiHost.hpp"
#include "canvas/api/ICanvasHost.hpp"
#include "canvas/api/ICanvasDocumentService.hpp"
#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/api/ICanvasStyleHost.hpp"
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
    struct RuntimeDependencies final {
        Core::IUiHost* uiHost = nullptr;
        Canvas::Api::ICanvasGridHost* gridHost = nullptr;
        Canvas::Api::ICanvasHost* canvasHost = nullptr;
        Canvas::Api::ICanvasDocumentService* canvasDocumentService = nullptr;
        Canvas::Api::ICanvasStyleHost* styleHost = nullptr;
        Core::IHeaderInfo* headerInfo = nullptr;
        ProjectExplorer::IProjectExplorer* projectExplorer = nullptr;
    };

    static QWidget* resolveDialogParent(Core::IUiHost* uiHost);

    Utils::Result resolveRuntimeDependencies(ExtensionSystem::PluginManager& manager, RuntimeDependencies& outDeps);
    Utils::Result configureService(const RuntimeDependencies& deps);
    void configureCanvasDefaults(const RuntimeDependencies& deps) const;
    Utils::Result configureDesignWorkflow(const RuntimeDependencies& deps);
    void configurePanelState();
    void registerSidebarTool(const RuntimeDependencies& deps);
    void connectHeaderInfo(const RuntimeDependencies& deps);
    void connectRibbonActions(const RuntimeDependencies& deps, ExtensionSystem::PluginManager& manager);
    void configureWorkspacePersistence(const RuntimeDependencies& deps);
    void restoreWorkspaceDesign();
    bool isRestorableBundlePath(const QString& bundlePath, const QString& workspaceRoot) const;
    void showOpenError(Core::IUiHost* uiHost, const QString& message) const;

    QPointer<Core::ISidebarRegistry> m_sidebarRegistry;
    QPointer<AieService> m_service;
    QPointer<AiePanelState> m_panelState;
    QPointer<DesignOpenController> m_designOpenController;
    std::unique_ptr<DesignBundleLoader> m_bundleLoader;
    std::unique_ptr<CanvasDocumentImporter> m_canvasImporter;
    AieWorkspaceState m_workspaceState;
    QMetaObject::Connection m_openFailedConnection;
    QMetaObject::Connection m_designOpenedConnection;
    QMetaObject::Connection m_designClosedConnection;
    QMetaObject::Connection m_newDesignTriggeredConnection;
    QMetaObject::Connection m_workspaceDesignOpenedConnection;
    QMetaObject::Connection m_workspaceRootChangedConnection;
    QString m_workspaceRoot;
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
    Q_UNUSED(manager);

    qInitResources_AiePluginResources();

    m_service = new AieService(this);
    ExtensionSystem::PluginManager::addObject(m_service);
    return Utils::Result::success();
}

void AiePlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
    if (!m_service) {
        qCWarning(aiepluginlog) << "AiePlugin: service unavailable during extensionsInitialized.";
        return;
    }

    RuntimeDependencies deps;
    const Utils::Result depsResult = resolveRuntimeDependencies(manager, deps);
    if (!depsResult) {
        logResultErrors(QStringLiteral("AiePlugin: missing required runtime dependencies."), depsResult);
        return;
    }

    const Utils::Result serviceResult = configureService(deps);
    if (!serviceResult) {
        logResultErrors(QStringLiteral("AiePlugin: failed to configure AIE service."), serviceResult);
        return;
    }

    configureCanvasDefaults(deps);

    const Utils::Result workflowResult = configureDesignWorkflow(deps);
    if (!workflowResult) {
        logResultErrors(QStringLiteral("AiePlugin: failed to configure design workflow."), workflowResult);
        return;
    }

    configurePanelState();
    registerSidebarTool(deps);
    connectHeaderInfo(deps);
    connectRibbonActions(deps, manager);
    configureWorkspacePersistence(deps);
}

ExtensionSystem::IPlugin::ShutdownFlag AiePlugin::aboutToShutdown()
{
    if (m_designOpenController && !m_workspaceRoot.isEmpty())
        m_workspaceState.setActiveBundlePathForRoot(m_workspaceRoot, m_designOpenController->activeBundlePath());

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

QWidget* AiePlugin::resolveDialogParent(Core::IUiHost* uiHost)
{
    if (uiHost)
        return uiHost->playgroundOverlayHost();
    return QApplication::activeWindow();
}

Utils::Result AiePlugin::resolveRuntimeDependencies(ExtensionSystem::PluginManager& manager, RuntimeDependencies& outDeps)
{
    RuntimeDependencies deps;
    deps.uiHost = manager.getObject<Core::IUiHost>();
    if (!deps.uiHost)
        return Utils::Result::failure(QStringLiteral("IUiHost is not available."));

    deps.gridHost = manager.getObject<Canvas::Api::ICanvasGridHost>();
    if (!deps.gridHost)
        return Utils::Result::failure(QStringLiteral("Canvas grid host is not available."));

    deps.canvasHost = manager.getObject<Canvas::Api::ICanvasHost>();
    deps.canvasDocumentService = manager.getObject<Canvas::Api::ICanvasDocumentService>();
    deps.styleHost = manager.getObject<Canvas::Api::ICanvasStyleHost>();
    deps.headerInfo = manager.getObject<Core::IHeaderInfo>();
    deps.projectExplorer = manager.getObject<ProjectExplorer::IProjectExplorer>();
    outDeps = deps;
    return Utils::Result::success();
}

Utils::Result AiePlugin::configureService(const RuntimeDependencies& deps)
{
    if (!m_service)
        return Utils::Result::failure(QStringLiteral("AIE service is not available."));

    m_service->setGridHost(deps.gridHost);
    m_service->setStyleHost(deps.styleHost);
    m_service->setCanvasHost(deps.canvasHost);

    const Utils::Result loadResult =
        m_service->loadProfileCatalog(QString::fromLatin1(Aie::kDeviceTopologiesResource));
    if (!loadResult)
        return loadResult;

    m_service->setBaseStyles(createDefaultBlockStyles());
    return Utils::Result::success();
}

void AiePlugin::configureCanvasDefaults(const RuntimeDependencies& deps) const
{
    if (!deps.canvasHost)
        return;

    deps.canvasHost->setEmptyStateText(QStringLiteral("No design open."),
                                       QStringLiteral("Create or open a design to start."));
    deps.canvasHost->setCanvasActive(false);
}

Utils::Result AiePlugin::configureDesignWorkflow(const RuntimeDependencies& deps)
{
    if (!m_service)
        return Utils::Result::failure(QStringLiteral("AIE service is not available."));

    if (!m_bundleLoader)
        m_bundleLoader = std::make_unique<DesignBundleLoader>(&m_service->catalog());
    if (!m_canvasImporter)
        m_canvasImporter = std::make_unique<CanvasDocumentImporter>(m_service);

    if (!deps.canvasDocumentService)
        return Utils::Result::failure(QStringLiteral("Canvas document service is not available."));

    if (!m_designOpenController) {
        m_designOpenController = new DesignOpenController(m_bundleLoader.get(),
                                                          m_canvasImporter.get(),
                                                          deps.canvasDocumentService,
                                                          this);
    }

    m_designOpenController->setProjectExplorer(deps.projectExplorer);

    if (m_openFailedConnection)
        disconnect(m_openFailedConnection);
    m_openFailedConnection = connect(m_designOpenController, &DesignOpenController::openFailed, this,
                                     [this, uiHost = QPointer<Core::IUiHost>(deps.uiHost)](const QString& message) {
                                         showOpenError(uiHost, message);
                                     });

    return Utils::Result::success();
}

void AiePlugin::configurePanelState()
{
    if (!m_service)
        return;
    if (!m_panelState)
        m_panelState = new AiePanelState(m_service->coordinator(), this);
    else
        m_panelState->setCoordinator(m_service->coordinator());
}

void AiePlugin::registerSidebarTool(const RuntimeDependencies& deps)
{
    if (!deps.uiHost)
        return;

    if (!m_sidebarRegistry)
        m_sidebarRegistry = deps.uiHost->sidebarRegistry();
    if (!m_sidebarRegistry || m_toolRegistered)
        return;

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
        return;
    }

    m_toolRegistered = true;
}

void AiePlugin::connectHeaderInfo(const RuntimeDependencies& deps)
{
    if (!deps.headerInfo || !m_designOpenController)
        return;

    if (m_designOpenedConnection)
        disconnect(m_designOpenedConnection);
    m_designOpenedConnection = connect(m_designOpenController, &DesignOpenController::designOpened, this,
                                       [this, header = QPointer<Core::IHeaderInfo>(deps.headerInfo)]
                                       (const QString&, const QString& displayName, const QString& deviceId) {
                                           if (!header)
                                               return;
                                           header->setDesignLabel(displayName);
                                           if (m_service)
                                               header->setDeviceLabel(formatDeviceLabel(m_service->catalog(), deviceId));
                                           else
                                               header->setDeviceLabel(deviceId);
                                       });

    if (m_designClosedConnection)
        disconnect(m_designClosedConnection);
    m_designClosedConnection = connect(m_designOpenController, &DesignOpenController::designClosed, this,
                                       [header = QPointer<Core::IHeaderInfo>(deps.headerInfo)](const QString&) {
                                           if (!header)
                                               return;
                                           header->setDesignLabel(QString());
                                           header->setDeviceLabel(QString());
                                       });
}

void AiePlugin::connectRibbonActions(const RuntimeDependencies& deps, ExtensionSystem::PluginManager& manager)
{
    if (!deps.uiHost)
        return;

    QAction* newDesignAction = deps.uiHost->ribbonCommand(Core::Constants::RIBBON_TAB_HOME,
                                                           Core::Constants::RIBBON_TAB_HOME_PROJECT_GROUP,
                                                           Core::Constants::PROJECT_NEW_ITEMID);
    if (!newDesignAction) {
        qCWarning(aiepluginlog) << "AiePlugin: New Design action not available.";
        return;
    }

    auto* managerPtr = &manager;
    if (m_newDesignTriggeredConnection)
        disconnect(m_newDesignTriggeredConnection);
    m_newDesignTriggeredConnection = connect(newDesignAction, &QAction::triggered, this,
                                             [this, managerPtr, uiHost = QPointer<Core::IUiHost>(deps.uiHost)]() {
                                                 QWidget* parent = resolveDialogParent(uiHost);

                                                 AieNewDesignDialog dialog(parent);
                                                 if (dialog.exec() != QDialog::Accepted)
                                                     return;

                                                 const auto result = dialog.result();
                                                 if (!result.created)
                                                     return;

                                                 if (auto* projectExplorer = managerPtr->getObject<ProjectExplorer::IProjectExplorer>())
                                                     projectExplorer->refresh();

                                                 if (m_designOpenController)
                                                     m_designOpenController->openBundlePath(result.bundlePath);
                                             });
}

void AiePlugin::configureWorkspacePersistence(const RuntimeDependencies& deps)
{
    if (!m_designOpenController)
        return;

    if (m_workspaceDesignOpenedConnection)
        disconnect(m_workspaceDesignOpenedConnection);
    m_workspaceDesignOpenedConnection = connect(m_designOpenController, &DesignOpenController::designOpened, this,
                                                [this](const QString& bundlePath, const QString&, const QString&) {
                                                    if (m_workspaceRoot.isEmpty())
                                                        return;
                                                    m_workspaceState.setActiveBundlePathForRoot(m_workspaceRoot, bundlePath);
                                                });

    if (m_workspaceRootChangedConnection)
        disconnect(m_workspaceRootChangedConnection);

    m_workspaceRoot = Utils::PathUtils::normalizePath(deps.projectExplorer ? deps.projectExplorer->rootPath()
                                                                            : QString());

    if (deps.projectExplorer) {
        m_workspaceRootChangedConnection = connect(deps.projectExplorer,
                                                   &ProjectExplorer::IProjectExplorer::workspaceRootChanged,
                                                   this,
                                                   [this](const QString& rootPath, bool userInitiated) {
                                                       Q_UNUSED(userInitiated);
                                                       if (!m_workspaceRoot.isEmpty() && m_designOpenController) {
                                                           const QString activeBundlePath =
                                                               m_designOpenController->activeBundlePath();
                                                           if (!activeBundlePath.isEmpty()) {
                                                               m_workspaceState.setActiveBundlePathForRoot(m_workspaceRoot,
                                                                                                           activeBundlePath);
                                                           }
                                                       }

                                                       m_workspaceRoot = Utils::PathUtils::normalizePath(rootPath);
                                                       restoreWorkspaceDesign();
                                                   });
    }

    restoreWorkspaceDesign();
}

void AiePlugin::restoreWorkspaceDesign()
{
    if (m_workspaceRoot.isEmpty() || !m_designOpenController)
        return;

    const QString persistedBundlePath = m_workspaceState.activeBundlePathForRoot(m_workspaceRoot);
    if (persistedBundlePath.isEmpty())
        return;

    if (!isRestorableBundlePath(persistedBundlePath, m_workspaceRoot)) {
        m_workspaceState.clearRoot(m_workspaceRoot);
        return;
    }

    const QString activeBundlePath = Utils::PathUtils::normalizePath(m_designOpenController->activeBundlePath());
#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif
    if (!activeBundlePath.isEmpty()
        && QString::compare(activeBundlePath, persistedBundlePath, pathCase) == 0) {
        return;
    }

    m_designOpenController->openBundlePath(persistedBundlePath);
}

bool AiePlugin::isRestorableBundlePath(const QString& bundlePath, const QString& workspaceRoot) const
{
    const QString normalizedBundlePath = Utils::PathUtils::normalizePath(bundlePath);
    const QString normalizedWorkspaceRoot = Utils::PathUtils::normalizePath(workspaceRoot);
    if (normalizedBundlePath.isEmpty() || normalizedWorkspaceRoot.isEmpty())
        return false;

    if (!Utils::DocumentBundle::hasBundleExtension(normalizedBundlePath))
        return false;

    const QFileInfo bundleInfo(normalizedBundlePath);
    if (!bundleInfo.exists() || !bundleInfo.isDir())
        return false;

    QString workspacePrefix = normalizedWorkspaceRoot;
    if (!workspacePrefix.endsWith('/'))
        workspacePrefix.append('/');

#if defined(Q_OS_WIN)
    return normalizedBundlePath.startsWith(workspacePrefix, Qt::CaseInsensitive);
#else
    return normalizedBundlePath.startsWith(workspacePrefix, Qt::CaseSensitive);
#endif
}

void AiePlugin::showOpenError(Core::IUiHost* uiHost, const QString& message) const
{
    if (message.trimmed().isEmpty())
        return;

    QMessageBox::warning(resolveDialogParent(uiHost), QStringLiteral("Open Design"), message);
}

} // namespace Aie::Internal

#include "AiePlugin.moc"
