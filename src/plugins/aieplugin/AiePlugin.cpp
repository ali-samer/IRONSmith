#include "aieplugin/AieGlobal.hpp"
#include "aieplugin/AieCanvasCoordinator.hpp"
#include "aieplugin/AieConstants.hpp"
#include "aieplugin/NpuProfileCanvasMapper.hpp"
#include "aieplugin/NpuProfileLoader.hpp"
#include "aieplugin/panels/AieToolPanel.hpp"
#include "aieplugin/state/AiePanelState.hpp"

#include <extensionsystem/IPlugin.hpp>
#include <utils/Result.hpp>

#include <QtCore/QLoggingCategory>
#include <QtCore/QStringList>

#include "core/api/ISidebarRegistry.hpp"
#include "core/api/SidebarToolSpec.hpp"
#include "core/ui/IUiHost.hpp"
#include "canvas/api/ICanvasGridHost.hpp"
#include "canvas/api/ICanvasStyleHost.hpp"
#include "canvas/api/CanvasStyleTypes.hpp"
#include "extensionsystem/PluginManager.hpp"

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
    QPointer<Core::ISidebarRegistry> m_sidebarRegistry;
    QPointer<AieCanvasCoordinator> m_canvasCoordinator;
    QPointer<AiePanelState> m_panelState;
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

} // namespace

Utils::Result AiePlugin::initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager)
{
    Q_UNUSED(arguments);
    Q_UNUSED(manager);
    qInitResources_AiePluginResources();
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
    auto* styleHost = manager.getObject<Canvas::Api::ICanvasStyleHost>();

    NpuProfileCatalog catalog;
    const Utils::Result loadResult =
        loadProfileCatalogFromFile(QString::fromLatin1(Aie::kDeviceTopologiesResource), catalog);
    if (!loadResult) {
        logResultErrors(QStringLiteral("AiePlugin: Failed to load NPU profile catalog."), loadResult);
        return;
    }

    const QString profileId = QString::fromLatin1(Aie::kDefaultProfileId);
    const NpuProfile* profile = findProfileById(catalog, profileId);
    if (!profile) {
        qCWarning(aiepluginlog) << "AiePlugin: Profile not found:" << profileId;
        return;
    }

    CanvasGridModel model;
    const Utils::Result buildResult = buildCanvasGridModel(*profile, model);
    if (!buildResult) {
        logResultErrors(QStringLiteral("AiePlugin: Failed to build canvas grid model."), buildResult);
        return;
    }

    if (!m_canvasCoordinator)
        m_canvasCoordinator = new AieCanvasCoordinator(this);
    m_canvasCoordinator->setGridHost(gridHost);
    m_canvasCoordinator->setStyleHost(styleHost);
    m_canvasCoordinator->setBaseModel(model);

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

    m_canvasCoordinator->setBaseStyles(baseStyles);

    if (!m_panelState)
        m_panelState = new AiePanelState(m_canvasCoordinator, this);
    else
        m_panelState->setCoordinator(m_canvasCoordinator);

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
            return new AieToolPanel(m_canvasCoordinator, parent);
        };

        QString error;
        if (!m_sidebarRegistry->registerTool(spec, factory, &error)) {
            qCWarning(aiepluginlog) << "AiePlugin: registerTool failed:" << error;
        } else {
            m_toolRegistered = true;
        }
    }
}

ExtensionSystem::IPlugin::ShutdownFlag AiePlugin::aboutToShutdown()
{
    if (m_sidebarRegistry && m_toolRegistered) {
        QString error;
        if (!m_sidebarRegistry->unregisterTool(QStringLiteral("IRONSmith.AieGridTools"), &error)) {
            qCWarning(aiepluginlog) << "AiePlugin: unregisterTool failed:" << error;
        }
    }
    return ShutdownFlag::SynchronousShutdown;
}

} // namespace Aie::Internal

#include "AiePlugin.moc"
