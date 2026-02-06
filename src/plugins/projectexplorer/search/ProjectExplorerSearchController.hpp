#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QVector>

class QKeyEvent;
class QLineEdit;
class QModelIndex;
class QTreeView;

namespace Utils {
class SidebarPanelFrame;
}

namespace ProjectExplorer::Internal {

class ProjectExplorerItemDelegate;
class ProjectExplorerService;
class ProjectExplorerSearchIndex;
class ProjectExplorerTreeState;

class ProjectExplorerSearchController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)

public:
    enum class MatchMode {
        Substring
    };
    Q_ENUM(MatchMode)

    ProjectExplorerSearchController(QTreeView* view,
                                    Utils::SidebarPanelFrame* frame,
                                    ProjectExplorerTreeState* treeState,
                                    ProjectExplorerService* service,
                                    ProjectExplorerSearchIndex* searchIndex,
                                    ProjectExplorerItemDelegate* delegate,
                                    QObject* parent = nullptr);

    QString searchText() const;
    void setSearchText(const QString& text);

    bool isActive() const;

    MatchMode matchMode() const;
    void setMatchMode(MatchMode mode);

public slots:
    void clearSearch();
    void refreshMatches();

signals:
    void searchTextChanged(const QString& text);
    void activeChanged(bool active);
    void matchModeChanged(MatchMode mode);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void beginSearch();
    void endSearch();
    void updateMatches();
    void snapshotState();
    void restoreState();
    void clearSearchExpansion();

    bool shouldStartFromKey(const QKeyEvent* event) const;
    void collectMatches(const QModelIndex& parent, QVector<QModelIndex>& matches) const;
    void expandAncestors(const QModelIndex& index);

    QPointer<QTreeView> m_view;
    QPointer<Utils::SidebarPanelFrame> m_frame;
    QPointer<ProjectExplorerTreeState> m_treeState;
    QPointer<ProjectExplorerService> m_service;
    QPointer<ProjectExplorerSearchIndex> m_searchIndex;
    QPointer<ProjectExplorerItemDelegate> m_delegate;
    QPointer<QLineEdit> m_searchField;

    QSet<QString> m_expandedSnapshot;
    QSet<QString> m_searchExpanded;
    bool m_rootExpandedSnapshot = true;
    QString m_currentSelectionPath;

    QString m_searchText;
    bool m_active = false;
    MatchMode m_matchMode = MatchMode::Substring;
};

} // namespace ProjectExplorer::Internal
