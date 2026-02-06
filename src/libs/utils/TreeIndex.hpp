#pragma once

#include "utils/TreeIds.hpp"

#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QVector>

#include <algorithm>
#include <utility>

namespace Utils {

template <typename Payload>
class TreeIndex final {
public:
    struct Node final {
        TreeNodeId id{};
        TreeNodeId parent{};
        QVector<TreeNodeId> children{};
        Payload payload{};
    };

    TreeIndex() = default;
    TreeIndex(const TreeIndex&) = delete;
    TreeIndex& operator=(const TreeIndex&) = delete;
    TreeIndex(TreeIndex&&) = default;
    TreeIndex& operator=(TreeIndex&&) = default;

    bool hasRoot() const noexcept { return !m_root.isNull(); }
    TreeNodeId rootId() const noexcept { return m_root; }

    TreeNodeId createRoot(Payload payload = {})
    {
        clear();
        TreeNodeId id = TreeNodeId::create();
        auto node = QSharedPointer<Node>::create();
        node->id = id;
        node->parent = TreeNodeId::null();
        node->payload = std::move(payload);
        m_nodes.insert(id, node);
        m_root = id;
        return id;
    }

    void clear()
    {
        m_nodes.clear();
        m_root = TreeNodeId::null();
    }

    int size() const noexcept { return m_nodes.size(); }
    bool contains(TreeNodeId id) const noexcept { return m_nodes.contains(id); }

    Node* node(TreeNodeId id)
    {
        auto it = m_nodes.find(id);
        return it == m_nodes.end() ? nullptr : it.value().data();
    }

    const Node* node(TreeNodeId id) const
    {
        auto it = m_nodes.find(id);
        return it == m_nodes.end() ? nullptr : it.value().data();
    }

    QVector<TreeNodeId> children(TreeNodeId id) const
    {
        const Node* n = node(id);
        return n ? n->children : QVector<TreeNodeId>{};
    }

    int childIndex(TreeNodeId parent, TreeNodeId child) const
    {
        const Node* n = node(parent);
        if (!n)
            return -1;
        const auto it = std::find(n->children.begin(), n->children.end(), child);
        if (it == n->children.end())
            return -1;
        return static_cast<int>(std::distance(n->children.begin(), it));
    }

    TreeNodeId addChild(TreeNodeId parent, Payload payload = {})
    {
        auto itParent = m_nodes.find(parent);
        if (itParent == m_nodes.end())
            return TreeNodeId::null();

        TreeNodeId id = TreeNodeId::create();
        auto node = QSharedPointer<Node>::create();
        node->id = id;
        node->parent = parent;
        node->payload = std::move(payload);
        m_nodes.insert(id, node);
        itParent.value()->children.push_back(id);
        return id;
    }

    bool removeSubtree(TreeNodeId id)
    {
        auto it = m_nodes.find(id);
        if (it == m_nodes.end())
            return false;

        const TreeNodeId parent = it.value()->parent;
        if (!parent.isNull()) {
            auto itParent = m_nodes.find(parent);
            if (itParent != m_nodes.end())
                removeChildRef(*itParent.value(), id);
        }

        removeSubtreeImpl(id);
        if (id == m_root)
            m_root = TreeNodeId::null();
        return true;
    }

    bool move(TreeNodeId id, TreeNodeId newParent, int index = -1)
    {
        if (id.isNull() || newParent.isNull())
            return false;
        if (id == m_root)
            return false;
        if (!contains(id) || !contains(newParent))
            return false;
        if (isAncestor(id, newParent))
            return false;

        auto itNode = m_nodes.find(id);
        auto itOldParent = m_nodes.find(itNode.value()->parent);
        auto itNewParent = m_nodes.find(newParent);
        if (itOldParent == m_nodes.end() || itNewParent == m_nodes.end())
            return false;

        removeChildRef(*itOldParent.value(), id);
        const int insertAt = clampIndex(index, itNewParent.value()->children.size());
        itNewParent.value()->children.insert(insertAt, id);
        itNode.value()->parent = newParent;
        return true;
    }

private:
    static int clampIndex(int index, int size)
    {
        if (index < 0 || index > size)
            return size;
        return index;
    }

    static void removeChildRef(Node& parent, TreeNodeId child)
    {
        const auto it = std::find(parent.children.begin(), parent.children.end(), child);
        if (it != parent.children.end())
            parent.children.erase(it);
    }

    bool isAncestor(TreeNodeId ancestor, TreeNodeId nodeId) const
    {
        TreeNodeId cur = nodeId;
        while (!cur.isNull()) {
            if (cur == ancestor)
                return true;
            const Node* n = node(cur);
            if (!n)
                break;
            cur = n->parent;
        }
        return false;
    }

    void removeSubtreeImpl(TreeNodeId id)
    {
        const Node* n = node(id);
        if (!n)
            return;

        const QVector<TreeNodeId> children = n->children;
        for (TreeNodeId child : children)
            removeSubtreeImpl(child);

        m_nodes.remove(id);
    }

    QMap<TreeNodeId, QSharedPointer<Node>> m_nodes;
    TreeNodeId m_root{};
};

} // namespace Utils
