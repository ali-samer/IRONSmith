// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <algorithm>
#include <functional>

namespace Utils::Async {

class UTILS_EXPORT DebouncedInvoker final : public QObject
{
    Q_OBJECT

public:
    explicit DebouncedInvoker(QObject* parent = nullptr)
        : QObject(parent)
    {
        m_timer.setSingleShot(true);
        connect(&m_timer, &QTimer::timeout, this, &DebouncedInvoker::fire);
    }

    explicit DebouncedInvoker(int delayMs, QObject* parent = nullptr)
        : DebouncedInvoker(parent)
    {
        setDelayMs(delayMs);
    }

    void setDelayMs(int ms)
    {
        m_timer.setInterval(std::max(ms, 0));
    }

    int delayMs() const
    {
        return m_timer.interval();
    }

    void setAction(std::function<void()> action)
    {
        m_action = std::move(action);
    }

    template <typename Fn>
    void trigger(Fn&& action)
    {
        setAction(std::forward<Fn>(action));
        trigger();
    }

    void trigger()
    {
        if (!m_action)
            return;
        m_timer.start();
    }

    void cancel()
    {
        m_timer.stop();
    }

    bool isPending() const
    {
        return m_timer.isActive();
    }

private:
    void fire()
    {
        if (m_action)
            m_action();
    }

    QTimer m_timer;
    std::function<void()> m_action;
};

} // namespace Utils::Async
