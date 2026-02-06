#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtWidgets/QStyledItemDelegate>

namespace ProjectExplorer::Internal {

class ProjectExplorerItemDelegate final : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ProjectExplorerItemDelegate(QObject* parent = nullptr);

    void setSearchText(const QString& text);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    QString m_searchText;
};

} // namespace ProjectExplorer::Internal
