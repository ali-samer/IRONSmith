// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "core/GlobalMenuBar.hpp"

namespace Core {

GlobalMenuBar::GlobalMenuBar(QObject* parent)
    : QObject(parent)
{
}

bool GlobalMenuBar::containsId(const QString& id) const
{
    for (const auto& it : m_items) {
        if (it.id() == id)
            return true;
    }
    return false;
}

int GlobalMenuBar::indexOf(const QString& id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].id() == id)
            return i;
    }
    return -1;
}

const GlobalMenuBarItem* GlobalMenuBar::itemById(const QString& id) const
{
    const int idx = indexOf(id);
    if (idx < 0)
        return nullptr;
    return &m_items[idx];
}

bool GlobalMenuBar::setActiveId(const QString& id)
{
    if (id.isEmpty())
        return false;

    if (!containsId(id))
        return false;

    if (m_activeId == id)
        return true;

    m_activeId = id;
    emit activeChanged(m_activeId);
    return true;
}

bool GlobalMenuBar::addItem(const GlobalMenuBarItem& item)
{
    if (!item.isValid())
        return false;
    if (containsId(item.id()))
        return false;

    m_items.push_back(item);
    emit changed();

    if (m_activeId.isEmpty()) {
        m_activeId = item.id();
        emit activeChanged(m_activeId);
    }

    return true;
}

bool GlobalMenuBar::addItem(QString id, QString title)
{
    return addItem(GlobalMenuBarItem(std::move(id), std::move(title)));
}

bool GlobalMenuBar::removeItem(const QString& id)
{
    const int idx = indexOf(id);
    if (idx < 0)
        return false;

    const bool removingActive = (m_activeId == id);

    m_items.removeAt(idx);
    emit changed();

    if (removingActive) {
        if (!m_items.isEmpty()) {
            m_activeId = m_items.front().id();
            emit activeChanged(m_activeId);
        } else {
            m_activeId.clear();
            emit activeChanged(m_activeId);
        }
    }

    return true;
}

void GlobalMenuBar::clear()
{
    const bool hadItems = !m_items.isEmpty();
    const bool hadActive = !m_activeId.isEmpty();

    m_items.clear();
    m_activeId.clear();

    if (hadItems)
        emit changed();
    if (hadActive)
        emit activeChanged(QString());
}

} // namespace Core
