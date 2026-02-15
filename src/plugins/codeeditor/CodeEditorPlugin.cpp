// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "codeeditor/CodeEditorGlobal.hpp"
#include "codeeditor/api/CodeEditorTypes.hpp"
#include "codeeditor/internal/CodeEditorServiceImpl.hpp"

#include <core/ui/IUiHost.hpp>
#include <extensionsystem/IPlugin.hpp>
#include <extensionsystem/PluginManager.hpp>
#include <utils/Result.hpp>

#include <QtCore/QPointer>
#include <QtCore/QStringList>
#include <QtCore/QLoggingCategory>

Q_LOGGING_CATEGORY(ceditorlog, "ironsmith.codeeditor")

namespace CodeEditor::Internal {

class CodeEditorPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.ironsmith.plugin" FILE "CodeEditor.json")

public:
    Utils::Result initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager) override;
    void extensionsInitialized(ExtensionSystem::PluginManager& manager) override;
    ShutdownFlag aboutToShutdown() override;

private:
    QPointer<CodeEditorServiceImpl> m_service;
};

Utils::Result CodeEditorPlugin::initialize(const QStringList& arguments, ExtensionSystem::PluginManager& manager)
{
    Q_UNUSED(arguments);
    Q_UNUSED(manager);

    qCInfo(ceditorlog) << "CodeEditorPlugin: initialize";

    qRegisterMetaType<CodeEditor::Api::CodeEditorSessionHandle>(
        "CodeEditor::Api::CodeEditorSessionHandle");
    qRegisterMetaType<CodeEditor::Api::CodeEditorCloseReason>(
        "CodeEditor::Api::CodeEditorCloseReason");

    m_service = new CodeEditorServiceImpl(this);
    ExtensionSystem::PluginManager::addObject(m_service);

    return Utils::Result::success();
}

void CodeEditorPlugin::extensionsInitialized(ExtensionSystem::PluginManager& manager)
{
    qCInfo(ceditorlog) << "CodeEditorPlugin: extensionsInitialized";
    if (m_service)
        m_service->setUiHost(ExtensionSystem::PluginManager::getObject<Core::IUiHost>());
}

ExtensionSystem::IPlugin::ShutdownFlag CodeEditorPlugin::aboutToShutdown()
{
    qCInfo(ceditorlog) << "CodeEditorPlugin: aboutToShutdown";
    if (m_service) {
        ExtensionSystem::PluginManager::removeObject(m_service);
        m_service = nullptr;
    }
    return ShutdownFlag::SynchronousShutdown;
}

} // namespace CodeEditor::Internal

#include "CodeEditorPlugin.moc"