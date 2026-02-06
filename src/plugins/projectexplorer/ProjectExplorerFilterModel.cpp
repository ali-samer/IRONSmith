#include "projectexplorer/ProjectExplorerFilterModel.hpp"

#include "projectexplorer/ProjectExplorerModel.hpp"

#include <QtCore/Qt>

namespace ProjectExplorer::Internal {

ProjectExplorerFilterModel::ProjectExplorerFilterModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    setRecursiveFilteringEnabled(false);
}

void ProjectExplorerFilterModel::setFilterText(QString text)
{
    const QString next = text.trimmed();
    if (next == m_filter)
        return;
    m_filter = next;
    invalidateFilter();
}

bool ProjectExplorerFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_filter.isEmpty())
        return true;

    QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!idx.isValid())
        return false;

    const QString name = sourceModel()->data(idx, Qt::DisplayRole).toString();
    if (name.contains(m_filter, Qt::CaseInsensitive))
        return true;

    const int childCount = sourceModel()->rowCount(idx);
    for (int i = 0; i < childCount; ++i) {
        if (filterAcceptsRow(i, idx))
            return true;
    }

    return false;
}

bool ProjectExplorerFilterModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    const bool leftFolder = sourceModel()->data(left, ProjectExplorerModel::IsFolderRole).toBool();
    const bool rightFolder = sourceModel()->data(right, ProjectExplorerModel::IsFolderRole).toBool();
    if (leftFolder != rightFolder)
        return leftFolder;

    const QString l = sourceModel()->data(left, Qt::DisplayRole).toString();
    const QString r = sourceModel()->data(right, Qt::DisplayRole).toString();
    return QString::localeAwareCompare(l, r) < 0;
}

} // namespace ProjectExplorer::Internal
