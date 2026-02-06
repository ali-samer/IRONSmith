#include "projectexplorer/search/ProjectExplorerSearchController.hpp"

#include "projectexplorer/views/ProjectExplorerItemDelegate.hpp"
#include "projectexplorer/ProjectExplorerService.hpp"
#include "projectexplorer/search/ProjectExplorerSearchIndex.hpp"
#include "projectexplorer/ProjectExplorerTreeState.hpp"
#include "projectexplorer/search/ProjectExplorerSearchMatcher.hpp"
#include "utils/ui/SidebarPanelFrame.hpp"

#include <QtCore/QEvent>
#include <QtCore/QSignalBlocker>
#include <QtCore/QVector>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QAbstractItemView>
#include <QtCore/QAbstractItemModel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeView>

namespace ProjectExplorer::Internal {

ProjectExplorerSearchController::ProjectExplorerSearchController(QTreeView* view,
                                                                 Utils::SidebarPanelFrame* frame,
                                                                 ProjectExplorerTreeState* treeState,
                                                                 ProjectExplorerService* service,
                                                                 ProjectExplorerSearchIndex* searchIndex,
                                                                 ProjectExplorerItemDelegate* delegate,
                                                                 QObject* parent)
    : QObject(parent)
    , m_view(view)
    , m_frame(frame)
    , m_treeState(treeState)
    , m_service(service)
    , m_searchIndex(searchIndex)
    , m_delegate(delegate)
{
    if (m_view)
        m_view->installEventFilter(this);

    if (m_frame)
        m_searchField = m_frame->searchField();

    if (m_view && m_view->model()) {
        connect(m_view->model(), &QAbstractItemModel::modelReset, this, [this]() {
            if (m_active)
                updateMatches();
        });
        connect(m_view->model(), &QAbstractItemModel::layoutChanged, this, [this]() {
            if (m_active)
                updateMatches();
        });
    }

    if (m_searchField) {
        m_searchField->installEventFilter(this);
        connect(m_searchField, &QLineEdit::textChanged,
                this, &ProjectExplorerSearchController::setSearchText);
    }
}

QString ProjectExplorerSearchController::searchText() const
{
    return m_searchText;
}

void ProjectExplorerSearchController::setSearchText(const QString& text)
{
    const QString cleaned = text;
    if (cleaned == m_searchText)
        return;

    m_searchText = cleaned;

    if (m_searchField && m_searchField->text() != cleaned) {
        const QSignalBlocker blocker(m_searchField);
        m_searchField->setText(cleaned);
    }

    if (m_searchText.trimmed().isEmpty()) {
        endSearch();
        return;
    }

    if (!m_active)
        beginSearch();

    updateMatches();
    emit searchTextChanged(m_searchText);
}

bool ProjectExplorerSearchController::isActive() const
{
    return m_active;
}

ProjectExplorerSearchController::MatchMode ProjectExplorerSearchController::matchMode() const
{
    return m_matchMode;
}

void ProjectExplorerSearchController::setMatchMode(MatchMode mode)
{
    if (m_matchMode == mode)
        return;
    m_matchMode = mode;
    emit matchModeChanged(m_matchMode);
    if (m_active)
        updateMatches();
}

void ProjectExplorerSearchController::clearSearch()
{
    endSearch();
}

void ProjectExplorerSearchController::refreshMatches()
{
    if (m_active)
        updateMatches();
}

bool ProjectExplorerSearchController::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            clearSearch();
            return m_active;
        }

        if (obj == m_view) {
            if (m_active && keyEvent->key() == Qt::Key_Backspace) {
                if (!m_searchText.isEmpty()) {
                    QString next = m_searchText;
                    next.chop(1);
                    setSearchText(next);
                }
                return true;
            }

            if (shouldStartFromKey(keyEvent)) {
                const QString typed = keyEvent->text();
                if (!typed.isEmpty())
                    setSearchText(m_searchText + typed);
                return true;
            }
        }

        if (obj == m_searchField && keyEvent->key() == Qt::Key_Escape) {
            clearSearch();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

void ProjectExplorerSearchController::beginSearch()
{
    if (m_active)
        return;

    m_active = true;
    m_searchExpanded.clear();
    snapshotState();

    if (m_treeState)
        m_treeState->setSuspended(true);

    if (m_frame)
        m_frame->setSearchEnabled(true);

    emit activeChanged(true);
}

void ProjectExplorerSearchController::endSearch()
{
    if (!m_active)
        return;

    m_active = false;
    m_searchExpanded.clear();

    if (m_searchField) {
        const QSignalBlocker blocker(m_searchField);
        m_searchField->clear();
    }

    m_searchText.clear();

    if (m_delegate)
        m_delegate->setSearchText(QString());

    if (m_view && m_view->viewport())
        m_view->viewport()->update();

    restoreState();

    if (m_frame)
        m_frame->setSearchEnabled(false);

    if (m_treeState)
        m_treeState->setSuspended(false);

    emit activeChanged(false);
    emit searchTextChanged(m_searchText);
}

void ProjectExplorerSearchController::updateMatches()
{
    if (!m_view || !m_view->model())
        return;

    if (m_delegate)
        m_delegate->setSearchText(m_searchText);

    const QString trimmed = m_searchText.trimmed();
    if (trimmed.isEmpty())
        return;

    clearSearchExpansion();

    QVector<QModelIndex> matches;
    if (m_searchIndex && m_searchIndex->isReady() && m_service) {
        const QVector<QString> paths = m_searchIndex->findMatches(m_searchText);
        matches.reserve(paths.size());
        for (const QString& path : paths) {
            const QModelIndex idx = m_service->indexForPath(path);
            if (idx.isValid())
                matches.push_back(idx);
        }
    } else {
        collectMatches(QModelIndex(), matches);
    }

    for (const QModelIndex& index : matches)
        expandAncestors(index);

    if (!matches.isEmpty())
        m_view->scrollTo(matches.first(), QAbstractItemView::PositionAtCenter);

    if (m_view->viewport())
        m_view->viewport()->update();
}

void ProjectExplorerSearchController::snapshotState()
{
    m_expandedSnapshot.clear();
    m_rootExpandedSnapshot = true;
    m_currentSelectionPath.clear();

    if (!m_view || !m_view->model() || !m_service)
        return;

    const QModelIndex rootIndex = m_view->model()->index(0, 0);
    if (rootIndex.isValid())
        m_rootExpandedSnapshot = m_view->isExpanded(rootIndex);

    const QModelIndex current = m_view->currentIndex();
    if (current.isValid())
        m_currentSelectionPath = m_service->pathForIndex(current);

    QVector<QModelIndex> stack;
    const int rootRows = m_view->model()->rowCount(QModelIndex());
    for (int row = 0; row < rootRows; ++row) {
        const QModelIndex idx = m_view->model()->index(row, 0, QModelIndex());
        if (idx.isValid())
            stack.push_back(idx);
    }

    while (!stack.isEmpty()) {
        const QModelIndex idx = stack.takeLast();
        if (!idx.isValid())
            continue;

        if (m_view->isExpanded(idx)) {
            const QString path = m_service->pathForIndex(idx);
            if (!path.isEmpty())
                m_expandedSnapshot.insert(path);
        }

        const int rows = m_view->model()->rowCount(idx);
        for (int row = 0; row < rows; ++row) {
            const QModelIndex child = m_view->model()->index(row, 0, idx);
            if (child.isValid())
                stack.push_back(child);
        }
    }
}

void ProjectExplorerSearchController::restoreState()
{
    if (!m_view || !m_view->model() || !m_service)
        return;

    const QModelIndex rootIndex = m_view->model()->index(0, 0);
    if (rootIndex.isValid())
        m_view->setExpanded(rootIndex, m_rootExpandedSnapshot);

    QVector<QModelIndex> stack;
    const int rootRows = m_view->model()->rowCount(QModelIndex());
    for (int row = 0; row < rootRows; ++row) {
        const QModelIndex idx = m_view->model()->index(row, 0, QModelIndex());
        if (idx.isValid())
            stack.push_back(idx);
    }

    while (!stack.isEmpty()) {
        const QModelIndex idx = stack.takeLast();
        if (!idx.isValid())
            continue;

        const QString path = m_service->pathForIndex(idx);
        if (!path.isEmpty()) {
            const bool shouldExpand = m_expandedSnapshot.contains(path);
            if (m_view->isExpanded(idx) != shouldExpand)
                m_view->setExpanded(idx, shouldExpand);
        }

        const int rows = m_view->model()->rowCount(idx);
        for (int row = 0; row < rows; ++row) {
            const QModelIndex child = m_view->model()->index(row, 0, idx);
            if (child.isValid())
                stack.push_back(child);
        }
    }

    if (!m_currentSelectionPath.isEmpty()) {
        const QModelIndex restored = m_service->indexForPath(m_currentSelectionPath);
        if (restored.isValid())
            m_view->setCurrentIndex(restored);
    }
}

void ProjectExplorerSearchController::clearSearchExpansion()
{
    if (!m_view || !m_view->model())
        return;

    for (const QString& path : m_searchExpanded) {
        if (m_service) {
            const QModelIndex idx = m_service->indexForPath(path);
            if (idx.isValid())
                m_view->setExpanded(idx, false);
        }
    }
    m_searchExpanded.clear();
}

bool ProjectExplorerSearchController::shouldStartFromKey(const QKeyEvent* event) const
{
    if (!event)
        return false;

    const Qt::KeyboardModifiers mods = event->modifiers();
    if (mods.testFlag(Qt::ControlModifier) || mods.testFlag(Qt::AltModifier) || mods.testFlag(Qt::MetaModifier))
        return false;

    const QString text = event->text();
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);
    if (ch.isNull())
        return false;
    if (!ch.isPrint())
        return false;
    return !ch.isSpace();
}

void ProjectExplorerSearchController::collectMatches(const QModelIndex& parent,
                                                     QVector<QModelIndex>& matches) const
{
    if (!m_view || !m_view->model())
        return;

    const int rows = m_view->model()->rowCount(parent);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex idx = m_view->model()->index(row, 0, parent);
        if (!idx.isValid())
            continue;

        const QString name = m_view->model()->data(idx, Qt::DisplayRole).toString();
        const auto result = ProjectExplorerSearchMatcher::match(name, m_searchText, Qt::CaseInsensitive);
        if (result.matched)
            matches.push_back(idx);

        collectMatches(idx, matches);
    }
}

void ProjectExplorerSearchController::expandAncestors(const QModelIndex& index)
{
    if (!m_view)
        return;

    QModelIndex parent = index.parent();
    while (parent.isValid()) {
        if (!m_view->isExpanded(parent)) {
            m_view->setExpanded(parent, true);
            if (m_service) {
                const QString path = m_service->pathForIndex(parent);
                if (!path.isEmpty())
                    m_searchExpanded.insert(path);
            }
        }
        parent = parent.parent();
    }
}

} // namespace ProjectExplorer::Internal
