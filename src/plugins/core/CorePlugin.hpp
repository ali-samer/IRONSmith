#pragma once

#include <QtCore/QPointer>
#include <QtCore/QStringList>

#include "extensionsystem/IPlugin.hpp"

namespace Core {
class CoreImpl;

class CorePlugin final : public ExtensionSystem::IPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "Core.json")

public:
	explicit CorePlugin(QObject* parent = nullptr);
	~CorePlugin() override;

	Utils::Result initialize(const QStringList& arguments,
							 ExtensionSystem::PluginManager& manager) override;

	void extensionsInitialized(ExtensionSystem::PluginManager& manager) override;

private:
	QPointer<CoreImpl> m_core;
};

} // namespace Core
