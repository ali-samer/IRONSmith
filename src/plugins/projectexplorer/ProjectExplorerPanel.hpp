// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QPersistentModelIndex>
#include <QtCore/QPointer>
#include <QtWidgets/QWidget>

class QTreeView;
class QModelIndex;
class QPoint;

namespace Utils {
class ContextMenu;
class SidebarPanelFrame;
}

namespace ProjectExplorer::Internal {

class ProjectExplorerService;
class ProjectExplorerSearchIndex;
class ProjectExplorerPanelState;
class ProjectExplorerTreeState;
class ProjectExplorerSearchController;

class ProjectExplorerPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectExplorerPanel(ProjectExplorerService* service, QWidget* parent = nullptr);

private slots:
    void collapseDescendants(const QModelIndex& index);
    void handleActivate(const QModelIndex& index);
    void handleSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void showContextMenu(const QPoint& pos);
    void handleContextAction(const QString& id);
    void handlePanelAction(const QString& id);
    void handleSelectPath(const QString& path);

private:
    QPointer<ProjectExplorerService> m_service;
    ProjectExplorerTreeState* m_treeState = nullptr;
    ProjectExplorerPanelState* m_panelState = nullptr;
    ProjectExplorerSearchIndex* m_searchIndex = nullptr;
    ProjectExplorerSearchController* m_searchController = nullptr;
    Utils::SidebarPanelFrame* m_frame = nullptr;
    QTreeView* m_tree = nullptr;
    QPersistentModelIndex m_contextIndex;
    Utils::ContextMenu* m_contextMenu = nullptr;
};

} // namespace ProjectExplorer::Internal
