// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "utils/UtilsGlobal.hpp"
#include "utils/filesystem/PathPatternMatcher.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <qnamespace.h>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
QT_END_NAMESPACE

namespace Utils {
class PathPatternMatcher;

class UTILS_EXPORT RecursiveFileSystemWatcher final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList rootPaths READ rootPaths WRITE setRootPaths NOTIFY rootPathsChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(int debounceMs READ debounceMs WRITE setDebounceMs NOTIFY debounceMsChanged)
    Q_PROPERTY(QStringList ignorePatterns READ ignorePatterns WRITE setIgnorePatterns NOTIFY ignorePatternsChanged)

public:
    explicit RecursiveFileSystemWatcher(QObject* parent = nullptr);

    QStringList rootPaths() const;
    void setRootPaths(const QStringList& roots);
    void setRootPath(const QString& root);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    int debounceMs() const;
    void setDebounceMs(int ms);

    QStringList ignorePatterns() const;
    void setIgnorePatterns(const QStringList& patterns);

signals:
    void rootPathsChanged(const QStringList& roots);
    void enabledChanged(bool enabled);
    void debounceMsChanged(int ms);
    void ignorePatternsChanged(const QStringList& patterns);
    void pathsChanged(const QStringList& paths);

private slots:
    void handleDirectoryChanged(const QString& path);
    void handleFileChanged(const QString& path);
    void flushChanges();
    void performRescan();

private:
    void scheduleRescan();
    void applyWatchSet(const QSet<QString>& directories);

    static QSet<QString> scanDirectories(const QStringList& roots,
                                         const Utils::PathPatternMatcher* matcher);
    static void scanDirectory(const QString& root,
                              const QString& dirPath,
                              const Utils::PathPatternMatcher* matcher,
                              QSet<QString>& directories);

    QStringList m_rootPaths;
    QPointer<QFileSystemWatcher> m_watcher;
    QSet<QString> m_watchedDirs;
    QSet<QString> m_pendingChanges;
    QTimer m_flushTimer;
    QTimer m_rescanTimer;

    bool m_enabled = true;
    bool m_rescanInFlight = false;
    bool m_rescanPending = false;
    int m_debounceMs = 200;
    quint64 m_generation = 0;
    Utils::PathPatternMatcher m_ignoreMatcher;
    QStringList m_ignorePatterns;
};

} // namespace Utils
