// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/ProjectExplorerModel.hpp"

#include <QtCore/QDir>
#include <QtCore/QStringList>

namespace ProjectExplorer::Internal {

ProjectExplorerModel::ProjectExplorerModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_rootLabel(QStringLiteral("Project"))
{
    setRootLabel(m_rootLabel);
}

void ProjectExplorerModel::setRootLabel(const QString& label)
{
    m_rootLabel = label.trimmed().isEmpty() ? QStringLiteral("Project") : label.trimmed();
    if (!m_tree.hasRoot()) {
        NodeData rootData;
        rootData.name = m_rootLabel;
        rootData.path = Utils::VirtualPath::fromBundle(QString());
        rootData.kind = NodeKind::Root;
        m_tree.createRoot(rootData);
    } else {
        if (auto* root = m_tree.node(m_tree.rootId())) {
            root->payload.name = m_rootLabel;
            root->payload.kind = NodeKind::Root;
        }
    }
    emit dataChanged(index(0, 0, QModelIndex()), index(0, 0, QModelIndex()));
}

QString ProjectExplorerModel::rootLabel() const
{
    return m_rootLabel;
}

void ProjectExplorerModel::setRootPath(const QString& path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned == m_rootPath)
        return;
    m_rootPath = cleaned;
    m_iconProvider.setRootPath(m_rootPath);
    if (m_tree.hasRoot())
        emit dataChanged(index(0, 0, QModelIndex()), index(0, 0, QModelIndex()), { RootPathRole });
}

QString ProjectExplorerModel::rootPath() const
{
    return m_rootPath;
}

void ProjectExplorerModel::setEntries(const ProjectExplorer::ProjectEntryList& entries)
{
    beginResetModel();
    m_entries = entries;
    rebuildTree();
    endResetModel();
}

ProjectExplorer::ProjectEntryList ProjectExplorerModel::entries() const
{
    return m_entries;
}

QModelIndex ProjectExplorerModel::indexForPath(const QString& path) const
{
    const QString key = Utils::VirtualPath::fromBundle(path).toString();
    auto it = m_pathIndex.find(key);
    if (it == m_pathIndex.end())
        return {};

    const auto* node = m_tree.node(it.value());
    if (!node)
        return {};

    if (node->parent.isNull())
        return createIndex(0, 0, const_cast<Utils::TreeIndex<NodeData>::Node*>(node));

    const auto* parentNode = m_tree.node(node->parent);
    if (!parentNode)
        return {};

    const int row = parentNode->children.indexOf(node->id);
    if (row < 0)
        return {};

    return createIndex(row, 0, const_cast<Utils::TreeIndex<NodeData>::Node*>(node));
}

int ProjectExplorerModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return m_tree.hasRoot() ? 1 : 0;

    const auto* node = nodeFromIndex(parent);
    if (!node)
        return 0;
    return node->children.size();
}

int ProjectExplorerModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QModelIndex ProjectExplorerModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0 || row < 0)
        return {};

    if (!parent.isValid()) {
        if (row != 0 || !m_tree.hasRoot())
            return {};
        auto* root = m_tree.node(m_tree.rootId());
        return root ? createIndex(0, 0, root) : QModelIndex{};
    }

    const auto* parentNode = nodeFromIndex(parent);
    if (!parentNode || row >= parentNode->children.size())
        return {};

    const Utils::TreeNodeId childId = parentNode->children[row];
    auto* child = m_tree.node(childId);
    return child ? createIndex(row, 0, child) : QModelIndex{};
}

QModelIndex ProjectExplorerModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return {};

    const auto* node = nodeFromIndex(index);
    if (!node)
        return {};

    if (node->parent.isNull())
        return {};

    const auto* parentNode = m_tree.node(node->parent);
    if (!parentNode)
        return {};

    if (parentNode->parent.isNull())
        return createIndex(0, 0, const_cast<Utils::TreeIndex<NodeData>::Node*>(parentNode));

    const auto* grandParent = m_tree.node(parentNode->parent);
    if (!grandParent)
        return {};

    const int row = grandParent->children.indexOf(parentNode->id);
    if (row < 0)
        return {};
    return createIndex(row, 0, const_cast<Utils::TreeIndex<NodeData>::Node*>(parentNode));
}

QVariant ProjectExplorerModel::data(const QModelIndex& index, int role) const
{
    const auto* node = nodeFromIndex(index);
    if (!node)
        return {};

    const NodeData& data = node->payload;
    if (role == Qt::DisplayRole)
        return data.name;

    if (role == PathRole)
        return data.path.toString();

    if (role == KindRole)
        return static_cast<int>(data.kind);

    if (role == IsFolderRole)
        return data.kind == NodeKind::Root || data.kind == NodeKind::Folder;

    if (role == RootPathRole && data.kind == NodeKind::Root)
        return QDir::toNativeSeparators(m_rootPath);

    if (role == Qt::DecorationRole)
        return m_iconProvider.iconForNode(static_cast<int>(data.kind), data.path, data.name);

    return {};
}

Qt::ItemFlags ProjectExplorerModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    const int kind = data(index, KindRole).toInt();
    if (kind != static_cast<int>(NodeKind::Root))
        flags |= Qt::ItemIsEditable;
    return flags;
}

void ProjectExplorerModel::rebuildTree()
{
    m_tree.clear();
    m_pathIndex.clear();

    NodeData rootData;
    rootData.name = m_rootLabel;
    rootData.path = Utils::VirtualPath::fromBundle(QString());
    rootData.kind = NodeKind::Root;
    const Utils::TreeNodeId rootId = m_tree.createRoot(rootData);
    m_pathIndex.insert(QString(), rootId);

    for (const auto& entry : m_entries) {
        if (entry.path.trimmed().isEmpty())
            continue;

        const Utils::VirtualPath path = Utils::VirtualPath::fromBundle(entry.path);
        Utils::TreeNodeId parentId = rootId;

        const QStringList parts = path.segments();
        QString curPath;
        for (int i = 0; i < parts.size(); ++i) {
            if (!curPath.isEmpty())
                curPath.append('/');
            curPath.append(parts[i]);

            const bool isLeaf = (i == parts.size() - 1);
            const QString name = parts[i];

            auto it = m_pathIndex.find(curPath);
            if (it != m_pathIndex.end()) {
                parentId = it.value();
                continue;
            }

            NodeData data;
            data.name = name;
            data.path = Utils::VirtualPath::fromBundle(curPath);
            data.kind = isLeaf ? mapEntryKind(entry, data.path) : NodeKind::Folder;

            const Utils::TreeNodeId id = m_tree.addChild(parentId, data);
            m_pathIndex.insert(curPath, id);
            parentId = id;
        }
    }
}

ProjectExplorerModel::NodeKind ProjectExplorerModel::mapEntryKind(const ProjectExplorer::ProjectEntry& entry,
                                                                  const Utils::VirtualPath& path)
{
    switch (entry.kind) {
        case ProjectExplorer::ProjectEntryKind::Folder: return NodeKind::Folder;
        case ProjectExplorer::ProjectEntryKind::Design: return NodeKind::Design;
        case ProjectExplorer::ProjectEntryKind::Asset: return NodeKind::Asset;
        case ProjectExplorer::ProjectEntryKind::Meta: return NodeKind::Meta;
        case ProjectExplorer::ProjectEntryKind::Cache: return NodeKind::Cache;
        case ProjectExplorer::ProjectEntryKind::Unknown: break;
    }

    const QString ext = path.extension().toLower();
    if (ext == "graphml" || ext == "ironsmith" || ext == "irondesign")
        return NodeKind::Design;
    if (ext == "json" || ext == "xml")
        return NodeKind::Asset;
    if (ext == "py" || ext == "cpp" || ext == "cxx" || ext == "cc")
        return NodeKind::Asset;
    if (ext == "cmake")
        return NodeKind::Asset;

    return NodeKind::File;
}

const Utils::TreeIndex<ProjectExplorerModel::NodeData>::Node*
ProjectExplorerModel::nodeFromIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return nullptr;
    return static_cast<const Utils::TreeIndex<NodeData>::Node*>(index.internalPointer());
}

} // namespace ProjectExplorer::Internal
