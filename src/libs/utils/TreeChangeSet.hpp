// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/TreeIds.hpp"
#include "utils/UtilsGlobal.hpp"

#include <QtCore/QVector>

namespace Utils {

struct UTILS_EXPORT TreeChange final {
    enum class Kind {
        Added,
        Removed,
        Updated,
        Moved
    };

    Kind kind = Kind::Updated;
    TreeNodeId id{};

    TreeNodeId parent{};
    int index = -1;

    TreeNodeId oldParent{};
    int oldIndex = -1;
};

class UTILS_EXPORT TreeChangeSet final {
public:
    const QVector<TreeChange>& changes() const noexcept { return m_changes; }
    bool empty() const noexcept { return m_changes.isEmpty(); }

    void clear() { m_changes.clear(); }

    void addAdded(TreeNodeId id, TreeNodeId parent, int index)
    {
        TreeChange c;
        c.kind = TreeChange::Kind::Added;
        c.id = id;
        c.parent = parent;
        c.index = index;
        m_changes.push_back(c);
    }

    void addRemoved(TreeNodeId id, TreeNodeId parent, int index)
    {
        TreeChange c;
        c.kind = TreeChange::Kind::Removed;
        c.id = id;
        c.parent = parent;
        c.index = index;
        m_changes.push_back(c);
    }

    void addUpdated(TreeNodeId id)
    {
        TreeChange c;
        c.kind = TreeChange::Kind::Updated;
        c.id = id;
        m_changes.push_back(c);
    }

    void addMoved(TreeNodeId id,
                  TreeNodeId oldParent,
                  int oldIndex,
                  TreeNodeId newParent,
                  int newIndex)
    {
        TreeChange c;
        c.kind = TreeChange::Kind::Moved;
        c.id = id;
        c.oldParent = oldParent;
        c.oldIndex = oldIndex;
        c.parent = newParent;
        c.index = newIndex;
        m_changes.push_back(c);
    }

private:
    QVector<TreeChange> m_changes;
};

} // namespace Utils
