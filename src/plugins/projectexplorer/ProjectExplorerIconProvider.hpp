// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtGui/QIcon>
#include <QtWidgets/QFileIconProvider>

namespace Utils {
class VirtualPath;
}

namespace ProjectExplorer::Internal {

class ProjectExplorerModel;

class ProjectExplorerIconProvider final
{
public:
    ProjectExplorerIconProvider();

    void setRootPath(QString rootPath);

    QIcon iconForNode(int nodeKind, const Utils::VirtualPath& path, const QString& name) const;

private:
    const QIcon& folderIcon() const;
    QIcon iconForFileName(QString name) const;
    QIcon iconForExtension(QString ext) const;
    QIcon iconForResource(const QString& resource) const;
    QIcon platformIconForPath(const Utils::VirtualPath& path) const;

    static const QMap<QString, QString>& extensionIconMap();
    static const QMap<QString, QString>& fileNameIconMap();

    QString m_rootPath;
    mutable QMap<QString, QIcon> m_resourceCache;
    mutable QMap<QString, QIcon> m_platformCache;
    QFileIconProvider m_platformProvider;
};

} // namespace ProjectExplorer::Internal
