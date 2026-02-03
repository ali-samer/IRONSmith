#include "canvas/CanvasPlugin.hpp"

#include "canvas/CanvasService.hpp"

#include <extensionsystem/PluginManager.hpp>

namespace Canvas {

CanvasPlugin::CanvasPlugin(QObject* parent)
    : ExtensionSystem::IPlugin(parent)
{
}

CanvasPlugin::~CanvasPlugin()
{
    if (m_service)
        ExtensionSystem::PluginManager::removeObject(m_service);
}

Utils::Result CanvasPlugin::initialize(const QStringList& arguments,
                                       ExtensionSystem::PluginManager& manager)
{
    Q_UNUSED(arguments);
    Q_UNUSED(manager);

    m_service = new CanvasService(this);
    ExtensionSystem::PluginManager::addObject(m_service);
    return Utils::Result::success();
}

void CanvasPlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
    Q_UNUSED(manager);
    if (m_service)
        m_service->wireIntoApplication();
}

ExtensionSystem::IPlugin::ShutdownFlag CanvasPlugin::aboutToShutdown()
{
    if (m_service) {
        ExtensionSystem::PluginManager::removeObject(m_service);
        m_service = nullptr;
    }
    return ShutdownFlag::SynchronousShutdown;
}

} // namespace Canvas
