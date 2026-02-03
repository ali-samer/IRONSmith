#include "canvas/CanvasService.hpp"

#include "canvas/CanvasControlsPanel.hpp"
#include "canvas/CanvasView.hpp"

#include <extensionsystem/PluginManager.hpp>

#include <core/ui/IUiHost.hpp>
#include <core/api/ISidebarRegistry.hpp>

#include <command/CommandDispatcher.hpp>
#include <command/BuiltInCommands.hpp>

#include <designmodel/DesignDocument.hpp>
#include <designmodel/DesignMetadata.hpp>
#include <designmodel/DesignSchemaVersion.hpp>

#include <QtCore/QDebug>

namespace Canvas {

CanvasService::CanvasService(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<Canvas::CanvasRenderOptions>("Canvas::CanvasRenderOptions");
}

CanvasService::~CanvasService()
{
    // QPointer members are QObject-owned by parents; no manual deletion required.
}

void CanvasService::setRenderOptions(CanvasRenderOptions opts)
{
    if (opts.showAnnotations == m_options.showAnnotations
        && opts.showFabric == m_options.showFabric
        && opts.showPortHotspots == m_options.showPortHotspots) {
        return;
    }
    m_options = opts;
    emit renderOptionsChanged(m_options);
    if (m_view)
        m_view->setRenderOptions(m_options);
}

void CanvasService::wireIntoApplication()
{
    m_ui = ExtensionSystem::PluginManager::getObject<Core::IUiHost>();
    if (!m_ui) {
        qWarning() << "CanvasService: IUiHost not found in object pool.";
        return;
    }

    m_sidebar = m_ui->sidebarRegistry();
    if (!m_sidebar) {
        qWarning() << "CanvasService: sidebar registry not available.";
        return;
    }

    m_dispatcher = ExtensionSystem::PluginManager::getObject<Command::CommandDispatcher>();
    if (!m_dispatcher) {
        qWarning() << "CanvasService: CommandDispatcher not found in object pool.";
        return;
    }

    ensureInitialDocument();

    if (!m_view) {
        m_view = new CanvasView(nullptr);
        m_view->setCommandDispatcher(m_dispatcher);
        m_view->setRenderOptions(m_options);
        m_view->setDocument(m_dispatcher->document());
        connect(this, &CanvasService::renderOptionsChanged,
                         m_view, &CanvasView::setRenderOptions);
        connect(m_dispatcher, &Command::CommandDispatcher::documentChanged,
                         m_view, &CanvasView::setDocument);
        m_ui->setPlaygroundCenterBase(m_view);
        m_view->setFocus(Qt::OtherFocusReason);
    }

    registerSidebarTools();
}

void CanvasService::ensureInitialDocument()
{
    if (m_dispatcher->document().isValid())
        return;

    auto md = DesignModel::DesignMetadata::createNew("Untitled", "User", "profile:stub");
    DesignModel::DesignDocument::Builder b(DesignModel::DesignSchemaVersion::current(), md);
    m_dispatcher->setDocument(b.freeze());
}

void CanvasService::registerSidebarTools()
{
    Core::SidebarToolSpec spec;
    spec.id = "canvas.controls";
    spec.title = "Canvas";
    spec.toolTip = "Canvas controls";
    spec.iconResource = ":/ui/icons/128x128/folder-yellow.png";
    spec.side = Core::SidebarSide::Left;
    spec.family = Core::SidebarFamily::Vertical;
    spec.region = Core::SidebarRegion::Exclusive;
    spec.rail = Core::SidebarRail::Top;
    spec.order = 50;

    QString err;
    m_sidebar->registerTool(spec,
                            [this](QWidget* parent) -> QWidget* {
                                return new CanvasControlsPanel(parent, this, m_dispatcher);
                            },
                            &err);
    if (!err.isEmpty())
        qWarning() << "CanvasService: failed to register tool:" << err;
}

} // namespace Canvas
