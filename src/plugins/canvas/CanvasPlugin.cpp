#include "canvas/CanvasGlobal.hpp"

#include <extensionsystem/IPlugin.hpp>
#include <utils/Result.hpp>

#include <QStringList>
#include <QLoggingCategory>

#include "canvas/internal/CanvasHostImpl.hpp"
#include "extensionsystem/PluginManager.hpp"

Q_LOGGING_CATEGORY(canvaslog, "ironsmith.canvas")

namespace Canvas::Internal {

class CanvasPlugin final : public ExtensionSystem::IPlugin {
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "Canvas.json")

public:
	CanvasPlugin() = default;

	Utils::Result initialize(const QStringList &arguments, ExtensionSystem::PluginManager &manager) override;

	void extensionsInitialized(ExtensionSystem::PluginManager& manager) override;
	ShutdownFlag aboutToShutdown() override;

private:
	QPointer<CanvasHostImpl> m_host;
};


Utils::Result CanvasPlugin::initialize(const QStringList &arguments, ExtensionSystem::PluginManager &manager) {
	Q_UNUSED(arguments);
	Q_UNUSED(manager);

	qCInfo(canvaslog) << "CanvasPlugin: initialize...";
	m_host = new CanvasHostImpl();
	if (!m_host) {
		qCInfo(canvaslog) << "Failed to create CanvasHostImpl";
		return Utils::Result::failure("Failed to create CanvasHostImpl");
	}

	ExtensionSystem::PluginManager::addObject(m_host);
	return Utils::Result::success();
}

void CanvasPlugin::extensionsInitialized(ExtensionSystem::PluginManager &manager) {
	qCInfo(canvaslog) << "CanvasPlugin: extensionsInitialized";
	if (!m_host) {
		qCWarning(canvaslog) << "CanvasPlugin: CanvasHost is null";
		return;
	}

	m_host->wireIntoApplication(manager);
}

ExtensionSystem::IPlugin::ShutdownFlag CanvasPlugin::aboutToShutdown() {
	if (m_host) {
		ExtensionSystem::PluginManager::removeObject(m_host);
		m_host = nullptr;
	}
	return ShutdownFlag::SynchronousShutdown;
}

} // Canvas::Internal

#include "CanvasPlugin.moc"