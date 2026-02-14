#include "aieplugin/design/DesignOpenController.hpp"

#include "aieplugin/design/CanvasDocumentImporter.hpp"
#include "aieplugin/design/DesignBundleLoader.hpp"
#include "aieplugin/design/DesignModel.hpp"
#include "aieplugin/design/DesignPersistenceController.hpp"

#include "projectexplorer/api/IProjectExplorer.hpp"
#include "projectexplorer/ProjectExplorerService.hpp"

#include "core/ui/IUiHost.hpp"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>

namespace Aie::Internal {

DesignOpenController::DesignOpenController(DesignBundleLoader* loader,
                                           CanvasDocumentImporter* importer,
                                           DesignPersistenceController* persistence,
                                           QObject* parent)
    : QObject(parent)
    , m_loader(loader)
    , m_importer(importer)
    , m_persistence(persistence)
{
}

void DesignOpenController::setProjectExplorer(ProjectExplorer::IProjectExplorer* explorer)
{
    if (m_explorer == explorer)
        return;

    if (m_explorer)
        disconnect(m_explorer, nullptr, this, nullptr);

    m_explorer = explorer;
    if (!m_explorer)
        return;

    connect(m_explorer, &ProjectExplorer::IProjectExplorer::openRequested,
            this, &DesignOpenController::handleOpenRequested);
}

void DesignOpenController::setUiHost(Core::IUiHost* uiHost)
{
    m_uiHost = uiHost;
}

void DesignOpenController::openBundlePath(const QString& absolutePath)
{
    openBundleInternal(absolutePath);
}

void DesignOpenController::handleOpenRequested(const QString& path, ProjectExplorer::ProjectEntryKind kind)
{
    if (kind != ProjectExplorer::ProjectEntryKind::Design)
        return;

    const QString absolute = resolveAbsolutePath(path);
    if (absolute.isEmpty()) {
        showError(QStringLiteral("Unable to resolve design path."));
        return;
    }

    openBundleInternal(absolute);
}

QString DesignOpenController::resolveAbsolutePath(const QString& relPath) const
{
    const QFileInfo info(relPath);
    if (info.isAbsolute())
        return QDir::cleanPath(info.absoluteFilePath());

    const auto* svc = qobject_cast<const ProjectExplorer::Internal::ProjectExplorerService*>(m_explorer.data());
    if (!svc)
        return {};

    const QString root = svc->rootPath();
    if (root.isEmpty())
        return {};
    return QDir(root).filePath(relPath);
}

void DesignOpenController::openBundleInternal(const QString& absolutePath)
{
    if (!m_loader || !m_importer) {
        showError(QStringLiteral("Design loader is not available."));
        return;
    }

    if (m_persistence) {
        m_persistence->flush();
        m_persistence->suspend();
    }

    DesignModel model;
    const Utils::Result loadResult = m_loader->load(absolutePath, model);
    if (!loadResult) {
        if (m_persistence)
            m_persistence->resume();
        showError(loadResult.errors.join("\n"));
        return;
    }

    const Utils::Result importResult = m_importer->importDesign(model);
    if (!importResult) {
        if (m_persistence)
            m_persistence->resume();
        showError(importResult.errors.join("\n"));
        return;
    }

    if (m_persistence) {
        m_persistence->setActiveBundle(model.bundlePath, model.design);
        m_persistence->resume();
    }

    const QString displayName = QFileInfo(model.bundlePath).fileName();
    emit designOpened(model.bundlePath, displayName, model.deviceId);
}

QWidget* DesignOpenController::dialogParent() const
{
    if (m_uiHost)
        return m_uiHost->playgroundOverlayHost();
    return QApplication::activeWindow();
}

void DesignOpenController::showError(const QString& message) const
{
    if (message.trimmed().isEmpty())
        return;
    QMessageBox::warning(dialogParent(), QStringLiteral("Open Design"), message);
}

} // namespace Aie::Internal
