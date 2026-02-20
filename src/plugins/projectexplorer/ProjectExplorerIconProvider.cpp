// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/ProjectExplorerIconProvider.hpp"

#include "projectexplorer/ProjectExplorerModel.hpp"

#include <utils/VirtualPath.hpp>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

namespace ProjectExplorer::Internal {

ProjectExplorerIconProvider::ProjectExplorerIconProvider() = default;

void ProjectExplorerIconProvider::setRootPath(QString rootPath)
{
    const QString cleaned = QDir::cleanPath(rootPath);
    if (cleaned == m_rootPath)
        return;

    m_rootPath = cleaned;
    m_platformCache.clear();
}

QIcon ProjectExplorerIconProvider::iconForNode(int nodeKind, const Utils::VirtualPath& path, const QString& name) const
{
    const auto kind = static_cast<ProjectExplorerModel::NodeKind>(nodeKind);
    if (kind == ProjectExplorerModel::NodeKind::Root || kind == ProjectExplorerModel::NodeKind::Folder)
        return folderIcon();

    if (const QIcon byName = iconForFileName(name); !byName.isNull())
        return byName;

    if (const QIcon byExt = iconForExtension(path.extension()); !byExt.isNull())
        return byExt;

    return platformIconForPath(path);
}

const QIcon& ProjectExplorerIconProvider::folderIcon() const
{
    static QIcon icon;
    if (icon.isNull()) {
        icon.addFile(":/ui/icons/svg/folder.svg", QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(":/ui/icons/svg/opened_folder.svg", QSize(), QIcon::Normal, QIcon::On);
    }
    return icon;
}

QIcon ProjectExplorerIconProvider::iconForFileName(QString name) const
{
    name = name.trimmed().toLower();
    if (name.isEmpty())
        return {};

    const auto it = fileNameIconMap().find(name);
    if (it == fileNameIconMap().end())
        return {};

    return iconForResource(it.value());
}

QIcon ProjectExplorerIconProvider::iconForExtension(QString ext) const
{
    ext = ext.trimmed().toLower();
    if (ext.isEmpty())
        return {};

    const auto it = extensionIconMap().find(ext);
    if (it == extensionIconMap().end())
        return {};

    return iconForResource(it.value());
}

QIcon ProjectExplorerIconProvider::iconForResource(const QString& resource) const
{
    if (resource.isEmpty())
        return {};

    const auto it = m_resourceCache.find(resource);
    if (it != m_resourceCache.end())
        return it.value();

    QIcon icon(resource);
    m_resourceCache.insert(resource, icon);
    return icon;
}

QIcon ProjectExplorerIconProvider::platformIconForPath(const Utils::VirtualPath& path) const
{
    const QString extKey = path.extension().toLower();
    const QString nameKey = path.basename().toLower();
    const QString cacheKey = extKey.isEmpty() ? nameKey : extKey;

    if (!cacheKey.isEmpty()) {
        const auto it = m_platformCache.find(cacheKey);
        if (it != m_platformCache.end())
            return it.value();
    }

    QIcon icon;
    const QString relPath = path.toString();
    if (!m_rootPath.isEmpty() && !relPath.isEmpty()) {
        const QString absolutePath = QDir(m_rootPath).filePath(relPath);
        icon = m_platformProvider.icon(QFileInfo(absolutePath));
    } else {
        icon = m_platformProvider.icon(QFileIconProvider::File);
    }

    if (!cacheKey.isEmpty())
        m_platformCache.insert(cacheKey, icon);

    return icon;
}

const QMap<QString, QString>& ProjectExplorerIconProvider::extensionIconMap()
{
    static const QMap<QString, QString> map = {
        { QStringLiteral("cmake"), QStringLiteral(":/ui/icons/svg/cmake_icon.svg") },
        { QStringLiteral("cpp"), QStringLiteral(":/ui/icons/svg/cpp_icon.svg") },
        { QStringLiteral("cc"), QStringLiteral(":/ui/icons/svg/cpp_icon.svg") },
        { QStringLiteral("cxx"), QStringLiteral(":/ui/icons/svg/cpp_icon.svg") },
        { QStringLiteral("c"), QStringLiteral(":/ui/icons/svg/c_icon.svg") },
        { QStringLiteral("h"), QStringLiteral(":/ui/icons/svg/h_icon.svg") },
        { QStringLiteral("hpp"), QStringLiteral(":/ui/icons/svg/h_icon.svg") },
        { QStringLiteral("json"), QStringLiteral(":/ui/icons/svg/json_icon.svg") },
        { QStringLiteral("xml"), QStringLiteral(":/ui/icons/svg/xml_icon.svg") },
        { QStringLiteral("py"), QStringLiteral(":/ui/icons/svg/python_icon.svg") },
        { QStringLiteral("ironsmith"), QStringLiteral(":/ui/icons/svg/hammer_icon.svg") },
        { QStringLiteral("irondesign"), QStringLiteral(":/ui/icons/svg/hammer_icon.svg") },
        { QStringLiteral("graphml"), QStringLiteral(":/ui/icons/svg/graphml_icon.svg") },
        { QStringLiteral("md"), QStringLiteral(":/ui/icons/svg/markdown_icon.svg") },
        { QStringLiteral("markdown"), QStringLiteral(":/ui/icons/svg/markdown_icon.svg") },
        { QStringLiteral("txt"), QStringLiteral(":/ui/icons/svg/text_file_icon.svg") },
        { QStringLiteral("log"), QStringLiteral(":/ui/icons/svg/text_file_icon.svg") },
        { QStringLiteral("ini"), QStringLiteral(":/ui/icons/svg/text_file_icon.svg") }
    };
    return map;
}

const QMap<QString, QString>& ProjectExplorerIconProvider::fileNameIconMap()
{
    static const QMap<QString, QString> map = {
        { QStringLiteral("cmakelists.txt"), QStringLiteral(":/ui/icons/svg/cmake_icon.svg") }
    };
    return map;
}

} // namespace ProjectExplorer::Internal
