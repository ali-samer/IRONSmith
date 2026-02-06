#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtGui/QIcon>
#include <QtWidgets/QMenu>

namespace Utils {

struct UTILS_EXPORT ContextMenuAction final {
    QString id;
    QString text;
    QIcon icon;
    bool enabled = true;
    bool checkable = false;
    bool checked = false;
    bool isSeparator = false;

    static ContextMenuAction item(QString id, QString text, QIcon icon = {})
    {
        ContextMenuAction action;
        action.id = std::move(id);
        action.text = std::move(text);
        action.icon = std::move(icon);
        return action;
    }

    static ContextMenuAction separatorAction()
    {
        ContextMenuAction action;
        action.isSeparator = true;
        return action;
    }
};

class UTILS_EXPORT ContextMenu final : public QMenu
{
    Q_OBJECT

public:
    explicit ContextMenu(QWidget* parent = nullptr);

    void setActions(const QList<ContextMenuAction>& actions);
    QList<ContextMenuAction> actionsSpec() const;

signals:
    void actionTriggered(const QString& id);

private slots:
    void handleActionTriggered();

private:
    void rebuild();

    QList<ContextMenuAction> m_actions;
};

} // namespace Utils
