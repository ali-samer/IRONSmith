#include "utils/filesystem/RecursiveFileSystemWatcher.hpp"

#include "utils/async/AsyncTask.hpp"
#include "utils/filesystem/PathPatternMatcher.hpp"

#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QFileSystemWatcher>

namespace Utils {

namespace {

QString normalizeRoot(const QString& path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned.isEmpty())
        return {};
    return QFileInfo(cleaned).absoluteFilePath();
}

} // namespace

RecursiveFileSystemWatcher::RecursiveFileSystemWatcher(QObject* parent)
    : QObject(parent)
    , m_watcher(new QFileSystemWatcher(this))
{
    m_flushTimer.setSingleShot(true);
    m_rescanTimer.setSingleShot(true);
    m_flushTimer.setInterval(m_debounceMs);
    m_rescanTimer.setInterval(m_debounceMs);

    connect(&m_flushTimer, &QTimer::timeout, this, &RecursiveFileSystemWatcher::flushChanges);
    connect(&m_rescanTimer, &QTimer::timeout, this, &RecursiveFileSystemWatcher::performRescan);

    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &RecursiveFileSystemWatcher::handleDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &RecursiveFileSystemWatcher::handleFileChanged);
}

QStringList RecursiveFileSystemWatcher::ignorePatterns() const
{
    return m_ignorePatterns;
}

void RecursiveFileSystemWatcher::setIgnorePatterns(const QStringList& patterns)
{
    QStringList cleaned;
    cleaned.reserve(patterns.size());
    for (const QString& raw : patterns) {
        const QString pattern = raw.trimmed();
        if (!pattern.isEmpty())
            cleaned.push_back(pattern);
    }

    if (cleaned == m_ignorePatterns)
        return;

    m_ignorePatterns = cleaned;
    m_ignoreMatcher.setPatterns(m_ignorePatterns);
    emit ignorePatternsChanged(m_ignorePatterns);
    scheduleRescan();
}

QStringList RecursiveFileSystemWatcher::rootPaths() const
{
    return m_rootPaths;
}

void RecursiveFileSystemWatcher::setRootPaths(const QStringList& roots)
{
    QSet<QString> unique;
    for (const QString& root : roots) {
        const QString cleaned = normalizeRoot(root);
        if (!cleaned.isEmpty())
            unique.insert(cleaned);
    }

    QStringList next = unique.values();
    next.sort();

    if (next == m_rootPaths)
        return;

    m_rootPaths = next;
    emit rootPathsChanged(m_rootPaths);

    scheduleRescan();
}

void RecursiveFileSystemWatcher::setRootPath(const QString& root)
{
    setRootPaths(QStringList{root});
}

bool RecursiveFileSystemWatcher::isEnabled() const
{
    return m_enabled;
}

void RecursiveFileSystemWatcher::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged(m_enabled);

    if (!m_enabled) {
        if (m_watcher)
            m_watcher->removePaths(m_watcher->directories());
        m_watchedDirs.clear();
        m_pendingChanges.clear();
        m_flushTimer.stop();
        m_rescanTimer.stop();
        return;
    }

    scheduleRescan();
}

int RecursiveFileSystemWatcher::debounceMs() const
{
    return m_debounceMs;
}

void RecursiveFileSystemWatcher::setDebounceMs(int ms)
{
    const int next = qMax(0, ms);
    if (next == m_debounceMs)
        return;

    m_debounceMs = next;
    m_flushTimer.setInterval(m_debounceMs);
    m_rescanTimer.setInterval(m_debounceMs);
    emit debounceMsChanged(m_debounceMs);
}

void RecursiveFileSystemWatcher::handleDirectoryChanged(const QString& path)
{
    if (!m_enabled)
        return;
    m_pendingChanges.insert(path);
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void RecursiveFileSystemWatcher::handleFileChanged(const QString& path)
{
    if (!m_enabled)
        return;
    m_pendingChanges.insert(path);
    if (!m_flushTimer.isActive())
        m_flushTimer.start();
}

void RecursiveFileSystemWatcher::flushChanges()
{
    if (!m_enabled)
        return;

    const QStringList changed = m_pendingChanges.values();
    m_pendingChanges.clear();

    if (!changed.isEmpty())
        emit pathsChanged(changed);

    scheduleRescan();
}

void RecursiveFileSystemWatcher::scheduleRescan()
{
    if (!m_enabled)
        return;

    if (m_rescanInFlight) {
        m_rescanPending = true;
        return;
    }

    if (!m_rescanTimer.isActive())
        m_rescanTimer.start();
}

void RecursiveFileSystemWatcher::performRescan()
{
    if (!m_enabled)
        return;

    const QStringList roots = m_rootPaths;
    const quint64 token = ++m_generation;
    m_rescanInFlight = true;

    Utils::Async::run<QSet<QString>>(this,
                                     [roots, matcher = m_ignoreMatcher]() { return scanDirectories(roots, &matcher); },
                                     [this, token](QSet<QString> dirs) {
                                         if (token != m_generation)
                                             return;
                                         applyWatchSet(dirs);
                                         m_rescanInFlight = false;
                                         if (m_rescanPending) {
                                             m_rescanPending = false;
                                             scheduleRescan();
                                         }
                                     });
}

void RecursiveFileSystemWatcher::applyWatchSet(const QSet<QString>& directories)
{
    if (!m_watcher)
        return;

    const QSet<QString> next = directories;

    QSet<QString> toRemove = m_watchedDirs;
    toRemove.subtract(next);

    QSet<QString> toAdd = next;
    toAdd.subtract(m_watchedDirs);

    if (!toRemove.isEmpty())
        m_watcher->removePaths(toRemove.values());

    if (!toAdd.isEmpty())
        m_watcher->addPaths(toAdd.values());

    m_watchedDirs = next;
}

QSet<QString> RecursiveFileSystemWatcher::scanDirectories(const QStringList& roots,
                                                          const Utils::PathPatternMatcher* matcher)
{
    QSet<QString> dirs;
    for (const QString& root : roots) {
        if (root.isEmpty())
            continue;

        QFileInfo rootInfo(root);
        if (!rootInfo.exists() || !rootInfo.isDir())
            continue;

        dirs.insert(rootInfo.absoluteFilePath());
        scanDirectory(rootInfo.absoluteFilePath(), rootInfo.absoluteFilePath(), matcher, dirs);
    }

    return dirs;
}

void RecursiveFileSystemWatcher::scanDirectory(const QString& root,
                                               const QString& dirPath,
                                               const Utils::PathPatternMatcher* matcher,
                                               QSet<QString>& directories)
{
    QDir dir(dirPath);
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (entries.isEmpty())
        return;

    QDir rootDir(root);

    for (const QFileInfo& info : entries) {
        const QString rel = QDir::fromNativeSeparators(rootDir.relativeFilePath(info.filePath()));
        if (matcher && matcher->matches(rel, /*isDir=*/true))
            continue;

        directories.insert(info.absoluteFilePath());
        scanDirectory(root, info.absoluteFilePath(), matcher, directories);
    }
}

} // namespace Utils
