// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "aieplugin/design/DesignOpenController.hpp"

#include "aieplugin/design/CanvasDocumentImporter.hpp"
#include "aieplugin/design/DesignBundleLoader.hpp"
#include "aieplugin/design/DesignModel.hpp"
#include "aieplugin/design/DesignPersistenceController.hpp"

#include "projectexplorer/api/IProjectExplorer.hpp"
#include <utils/ScopeGuard.hpp>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

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
        emit openFailed(QStringLiteral("Unable to resolve design path."));
        return;
    }

    openBundleInternal(absolute);
}

QString DesignOpenController::resolveAbsolutePath(const QString& relPath) const
{
    const QFileInfo info(relPath);
    if (info.isAbsolute())
        return QDir::cleanPath(info.absoluteFilePath());

    if (!m_explorer)
        return {};

    const QString root = m_explorer->rootPath();
    if (root.isEmpty())
        return {};
    return QDir::cleanPath(QDir(root).filePath(relPath));
}

void DesignOpenController::openBundleInternal(const QString& absolutePath)
{
    if (!m_loader || !m_importer) {
        emit openFailed(QStringLiteral("Design loader is not available."));
        return;
    }

    if (m_persistence)
        m_persistence->flush();

    if (m_persistence)
        m_persistence->suspend();

    [[maybe_unused]] auto resumePersistence = Utils::makeScopeGuard([this]() {
        if (m_persistence)
            m_persistence->resume();
    });

    DesignModel model;
    const Utils::Result loadResult = m_loader->load(absolutePath, model);
    if (!loadResult) {
        emit openFailed(loadResult.errors.join("\n"));
        return;
    }

    const Utils::Result importResult = m_importer->importDesign(model);
    if (!importResult) {
        emit openFailed(importResult.errors.join("\n"));
        return;
    }

    if (m_persistence)
        m_persistence->setActiveBundle(model.bundlePath, model.design);

    const QString displayName = QFileInfo(model.bundlePath).fileName();
    emit designOpened(model.bundlePath, displayName, model.deviceId);
}

} // namespace Aie::Internal
