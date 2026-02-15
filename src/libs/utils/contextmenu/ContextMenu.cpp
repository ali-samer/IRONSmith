// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/contextmenu/ContextMenu.hpp"

#include <QtCore/QVariant>

namespace Utils {

ContextMenu::ContextMenu(QWidget* parent)
    : QMenu(parent)
{
    setSeparatorsCollapsible(false);
}

void ContextMenu::setActions(const QList<ContextMenuAction>& actions)
{
    m_actions = actions;
    rebuild();
}

QList<ContextMenuAction> ContextMenu::actionsSpec() const
{
    return m_actions;
}

void ContextMenu::handleActionTriggered()
{
    auto* action = qobject_cast<QAction*>(sender());
    if (!action)
        return;

    const QString id = action->data().toString();
    if (!id.isEmpty())
        emit actionTriggered(id);
}

void ContextMenu::rebuild()
{
    clear();

    for (const auto& spec : m_actions) {
        if (spec.isSeparator) {
            addSeparator();
            continue;
        }

        QAction* action = spec.icon.isNull() ? addAction(spec.text) : addAction(spec.icon, spec.text);
        action->setEnabled(spec.enabled);
        action->setCheckable(spec.checkable);
        action->setChecked(spec.checked);
        action->setData(spec.id);
        connect(action, &QAction::triggered, this, &ContextMenu::handleActionTriggered);
    }
}

} // namespace Utils
