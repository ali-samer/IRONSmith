// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtCore/QThreadPool>
#include <QtCore/QRunnable>
#include <QtCore/Qt>

#include <type_traits>
#include <utility>

namespace Utils::Async {

namespace detail {

template <typename Result, typename WorkFn, typename DoneFn>
class AsyncRunnable final : public QRunnable
{
public:
    AsyncRunnable(QPointer<QObject> context, WorkFn work, DoneFn done)
        : m_context(std::move(context))
        , m_work(std::move(work))
        , m_done(std::move(done))
    {
    }

    void run() override
    {
        if constexpr (std::is_void_v<Result>) {
            try {
                m_work();
            } catch (...) {
                return;
            }

            if (!m_context)
                return;

            QPointer<QObject> guard = m_context;
            auto done = std::move(m_done);
            QMetaObject::invokeMethod(guard, [guard, done = std::move(done)]() mutable {
                if (guard)
                    done();
            }, Qt::QueuedConnection);
        } else {
            Result result{};
            try {
                result = m_work();
            } catch (...) {
                return;
            }

            if (!m_context)
                return;

            QPointer<QObject> guard = m_context;
            auto done = std::move(m_done);
            QMetaObject::invokeMethod(guard,
                                      [guard, done = std::move(done), result = std::move(result)]() mutable {
                                          if (guard)
                                              done(std::move(result));
                                      },
                                      Qt::QueuedConnection);
        }
    }

private:
    QPointer<QObject> m_context;
    WorkFn m_work;
    DoneFn m_done;
};

} // namespace detail

template <typename Result, typename Work, typename Done>
void run(QObject* context, Work&& work, Done&& done, QThreadPool* pool = QThreadPool::globalInstance())
{
    using WorkFn = std::decay_t<Work>;
    using DoneFn = std::decay_t<Done>;

    if (!context || !pool)
        return;

    auto task = new detail::AsyncRunnable<Result, WorkFn, DoneFn>(
        QPointer<QObject>(context),
        WorkFn(std::forward<Work>(work)),
        DoneFn(std::forward<Done>(done)));
    task->setAutoDelete(true);
    pool->start(task);
}

} // namespace Utils::Async
