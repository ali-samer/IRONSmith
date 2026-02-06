#include "CommandPlugin.hpp"

#include "command/CommandDispatcher.hpp"
#include "command/CommandResult.hpp"
#include "command/BuiltInCommands.hpp"

#include <QtCore/QMetaType>

#include "extensionsystem/PluginManager.hpp"

namespace Command::Internal {

void CommandPlugin::registerMetaTypes()
{
    qRegisterMetaType<CommandResult>("Command::CommandResult");
    qRegisterMetaType<CreatedBlock>("Command::CreatedBlock");
    qRegisterMetaType<CreatedPort>("Command::CreatedPort");
    qRegisterMetaType<CreatedLink>("Command::CreatedLink");

    // when we emit DesignDocument in signals across threads later
    // qRegisterMetaType<DesignModel::DesignDocument>("DesignModel::DesignDocument");
}

Utils::Result CommandPlugin::initialize(const QStringList& /*arguments*/, ExtensionSystem::PluginManager& pluginManager)
{
    registerMetaTypes();

    m_dispatcher = new CommandDispatcher(this);
    pluginManager.addObject(m_dispatcher);


    return Utils::Result::success();
}

void CommandPlugin::extensionsInitialized(ExtensionSystem::PluginManager& /*pluginManager*/)
{
}

ExtensionSystem::IPlugin::ShutdownFlag CommandPlugin::aboutToShutdown()
{
    if (m_dispatcher) {
        ExtensionSystem::PluginManager::removeObject(m_dispatcher);
        m_dispatcher = nullptr;
    }
    return ShutdownFlag::SynchronousShutdown;
}

CommandDispatcher* CommandPlugin::dispatcher() const noexcept
{
    return m_dispatcher;
}

} // namespace Command::Internal
