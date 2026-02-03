#pragma once

#include <extensionsystem/IPlugin.hpp>

#include <QtCore/QPointer>

namespace Canvas {

class CanvasService;

class CanvasPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "Canvas.json")

public:
    explicit CanvasPlugin(QObject* parent = nullptr);
    ~CanvasPlugin() override;

    Utils::Result initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager) override;
    void extensionsInitialized(ExtensionSystem::PluginManager& manager) override;
    ShutdownFlag aboutToShutdown() override;

private:
    QPointer<CanvasService> m_service;
};

} // namespace Canvas
