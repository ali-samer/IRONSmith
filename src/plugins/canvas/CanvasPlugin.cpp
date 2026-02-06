#include "canvas/CanvasGlobal.hpp"

#include <extensionsystem/IPlugin.hpp>
#include <utils/Result.hpp>

#include <QStringList>
#include <QLoggingCategory>

#include "canvas/internal/CanvasHostImpl.hpp"
#include "extensionsystem/PluginManager.hpp"
#include "canvas/internal/CanvasGridHostImpl.hpp"
#include "canvas/internal/CanvasStyleHostImpl.hpp"
#include "canvas/CanvasView.hpp"
#include "canvas/CanvasTypes.hpp"

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
    QPointer<CanvasGridHostImpl> m_gridHost;
    QPointer<CanvasStyleHostImpl> m_styleHost;
};


Utils::Result CanvasPlugin::initialize(const QStringList &arguments, ExtensionSystem::PluginManager &manager) {
	Q_UNUSED(arguments);
	Q_UNUSED(manager);

	qCInfo(canvaslog) << "CanvasPlugin: initialize...";
	qRegisterMetaType<Canvas::ObjectId>("Canvas::ObjectId");
	qRegisterMetaType<Canvas::PortId>("Canvas::PortId");
	m_host = new CanvasHostImpl();
	if (!m_host) {
		qCInfo(canvaslog) << "Failed to create CanvasHostImpl";
		return Utils::Result::failure("Failed to create CanvasHostImpl");
	}

	ExtensionSystem::PluginManager::addObject(m_host);

	m_styleHost = new CanvasStyleHostImpl();
	ExtensionSystem::PluginManager::addObject(m_styleHost);
	return Utils::Result::success();
}

void CanvasPlugin::extensionsInitialized(ExtensionSystem::PluginManager &manager) {
	qCInfo(canvaslog) << "CanvasPlugin: extensionsInitialized";
	if (!m_host) {
		qCWarning(canvaslog) << "CanvasPlugin: CanvasHost is null";
		return;
	}

	m_host->wireIntoApplication(manager);

	if (!m_gridHost) {
		auto* view = qobject_cast<CanvasView*>(m_host->viewWidget());
		m_gridHost = new CanvasGridHostImpl(m_host->document(), view, m_styleHost, this);
		ExtensionSystem::PluginManager::addObject(m_gridHost);
	}
}

ExtensionSystem::IPlugin::ShutdownFlag CanvasPlugin::aboutToShutdown() {
	if (m_host) {
		ExtensionSystem::PluginManager::removeObject(m_host);
		m_host = nullptr;
	}
	if (m_gridHost) {
		ExtensionSystem::PluginManager::removeObject(m_gridHost);
		m_gridHost = nullptr;
	}
	if (m_styleHost) {
		ExtensionSystem::PluginManager::removeObject(m_styleHost);
		m_styleHost = nullptr;
	}
	return ShutdownFlag::SynchronousShutdown;
}

} // Canvas::Internal

#include "CanvasPlugin.moc"
