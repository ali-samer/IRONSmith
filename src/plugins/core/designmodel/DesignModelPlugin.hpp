#pragma once

#include <extensionsystem/IPlugin.hpp>
#include <QtCore/QObject>

namespace DesignModel::Internal {

class DesignModelPlugin final : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "DesignModel.json")
public:
	DesignModelPlugin() = default;
	~DesignModelPlugin() override = default;

	Utils::Result initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager) override;
	void extensionsInitialized(ExtensionSystem::PluginManager&) override;
	ShutdownFlag aboutToShutdown() override;
};

} // namespace DesignModel::Internal
