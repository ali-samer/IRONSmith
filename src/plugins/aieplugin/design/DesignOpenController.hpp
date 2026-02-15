// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/api/ProjectExplorerTypes.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>

namespace ProjectExplorer { class IProjectExplorer; }

namespace Aie::Internal {

class DesignBundleLoader;
class CanvasDocumentImporter;
class DesignPersistenceController;

class DesignOpenController final : public QObject
{
    Q_OBJECT

public:
    DesignOpenController(DesignBundleLoader* loader,
                         CanvasDocumentImporter* importer,
                         DesignPersistenceController* persistence,
                         QObject* parent = nullptr);

    void setProjectExplorer(ProjectExplorer::IProjectExplorer* explorer);

    void openBundlePath(const QString& absolutePath);

signals:
    void designOpened(const QString& bundlePath, const QString& displayName, const QString& deviceId);
    void openFailed(const QString& message);

private:
    void handleOpenRequested(const QString& path, ProjectExplorer::ProjectEntryKind kind);
    void openBundleInternal(const QString& absolutePath);
    QString resolveAbsolutePath(const QString& relPath) const;

    QPointer<ProjectExplorer::IProjectExplorer> m_explorer;
    DesignBundleLoader* m_loader = nullptr;
    CanvasDocumentImporter* m_importer = nullptr;
    DesignPersistenceController* m_persistence = nullptr;
};

} // namespace Aie::Internal
