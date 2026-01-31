#pragma once

#include <extensionsystem/IPlugin.hpp>
#include <QtCore/QPointer>

namespace Command {

class CommandDispatcher;

namespace Internal {

class CommandPlugin final : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "Command.json")

public:
	CommandPlugin() = default;
	~CommandPlugin() override = default;

	Utils::Result initialize(const QStringList& arguments, ExtensionSystem::PluginManager& pluginManager) override;
	void extensionsInitialized(ExtensionSystem::PluginManager& pluginManager) override;
	ShutdownFlag aboutToShutdown() override;

	CommandDispatcher* dispatcher() const noexcept;

private:
	void registerMetaTypes();

	QPointer<CommandDispatcher> m_dispatcher;
};

} // namespace Internal
} // namespace Command