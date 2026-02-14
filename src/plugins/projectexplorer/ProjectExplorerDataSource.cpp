#include "projectexplorer/ProjectExplorerDataSource.hpp"

#include <utils/async/AsyncTask.hpp>
#include <utils/DocumentBundle.hpp>
#include <utils/VirtualPath.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

#include <utility>

namespace ProjectExplorer::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kRootPathKey = u"projectExplorer/rootPath"_s;
const QString kIgnorePatternsKey = u"projectExplorer/ignorePatterns"_s;
const QString kIgnoreUseGitIgnoreKey = u"projectExplorer/useGitIgnore"_s;
const QString kIgnoreUseIronIgnoreKey = u"projectExplorer/useIronIgnore"_s;

const QStringList kDefaultIgnorePatterns = {
    QStringLiteral(".git"),
    QStringLiteral(".svn"),
    QStringLiteral(".hg"),
    QStringLiteral(".DS_Store"),
    QStringLiteral("build"),
    QStringLiteral("out"),
    QStringLiteral("cmake-build-*"),
    QStringLiteral("CMakeFiles"),
    QStringLiteral("node_modules"),
    QStringLiteral("__pycache__"),
    QStringLiteral("*.o"),
    QStringLiteral("*.obj"),
    QStringLiteral("*.tmp"),
    QStringLiteral("*.log")
};

struct ScanResult final {
    QString label;
    ProjectExplorer::ProjectEntryList entries;
};

} // namespace

ProjectExplorerDataSource::ProjectExplorerDataSource(QObject* parent)
    : QObject(parent)
    , m_env(makeEnvironment())
{
    initialize();
    m_watcher = new Utils::RecursiveFileSystemWatcher(this);
    connect(m_watcher, &Utils::RecursiveFileSystemWatcher::pathsChanged,
            this, [this](const QStringList&) { refresh(); });
    m_watcher->setIgnorePatterns(buildIgnorePatterns());
    if (!m_rootPath.isEmpty())
        m_watcher->setRootPath(m_rootPath);
}

ProjectExplorerDataSource::ProjectExplorerDataSource(Utils::Environment environment, QObject* parent)
    : QObject(parent)
    , m_env(std::move(environment))
{
    initialize();
    m_watcher = new Utils::RecursiveFileSystemWatcher(this);
    connect(m_watcher, &Utils::RecursiveFileSystemWatcher::pathsChanged,
            this, [this](const QStringList&) { refresh(); });
    m_watcher->setIgnorePatterns(buildIgnorePatterns());
    if (!m_rootPath.isEmpty())
        m_watcher->setRootPath(m_rootPath);
}

void ProjectExplorerDataSource::initialize()
{
    const QString saved = m_env.setting(Utils::EnvironmentScope::Global, kRootPathKey, QString()).toString();
    if (!saved.isEmpty()) {
        m_rootPath = saved;
    } else {
        m_rootPath = QDir::currentPath();
        m_env.setSetting(Utils::EnvironmentScope::Global, kRootPathKey, m_rootPath);
    }

    m_ignorePatterns = m_env.setting(Utils::EnvironmentScope::Global,
                                     kIgnorePatternsKey,
                                     QStringList()).toStringList();
    m_useGitIgnore = m_env.setting(Utils::EnvironmentScope::Global,
                                   kIgnoreUseGitIgnoreKey,
                                   true).toBool();
    m_useIronIgnore = m_env.setting(Utils::EnvironmentScope::Global,
                                    kIgnoreUseIronIgnoreKey,
                                    true).toBool();

    m_ignoreMatcher.setPatterns(buildIgnorePatterns());
    if (m_watcher)
        m_watcher->setIgnorePatterns(buildIgnorePatterns());
}

QString ProjectExplorerDataSource::rootPath() const
{
    return m_rootPath;
}

void ProjectExplorerDataSource::setRootPath(const QString& path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned.isEmpty() || cleaned == m_rootPath)
        return;

    m_rootPath = cleaned;
    m_env.setSetting(Utils::EnvironmentScope::Global, kRootPathKey, m_rootPath);
    emit rootPathChanged(m_rootPath);
    m_ignoreMatcher.setPatterns(buildIgnorePatterns());
    if (m_watcher)
        m_watcher->setRootPath(m_rootPath);
    refresh();
}

void ProjectExplorerDataSource::refresh()
{
    const QString rootPath = m_rootPath;
    const quint64 token = ++m_scanGeneration;
    const Utils::PathPatternMatcher matcher = m_ignoreMatcher;

    Utils::Async::run<ScanResult>(this,
      [rootPath, matcher]() {
          ScanResult result;
          QFileInfo rootInfo(rootPath);
          if (!rootInfo.exists() || !rootInfo.isDir()) {
              result.label = QStringLiteral("Project");
              return result;
          }

          result.label = rootInfo.fileName().isEmpty() ? rootInfo.absoluteFilePath()
                                                       : rootInfo.fileName();
          result.entries = scanEntries(rootPath, matcher);
          return result;
      },
      [this, token](ScanResult result) {
          if (token != m_scanGeneration.load())
              return;
          emit rootLabelChanged(result.label);
          emit entriesChanged(result.entries);
      });
}

Utils::Environment ProjectExplorerDataSource::makeEnvironment()
{
    Utils::EnvironmentConfig cfg;
    cfg.organizationName = QStringLiteral("IRONSmith");
    cfg.applicationName = QStringLiteral("IRONSmith");
    return Utils::Environment(cfg);
}

ProjectExplorer::ProjectEntryKind ProjectExplorerDataSource::classifyPath(const QString& relPath, bool isDir)
{
    if (isDir) {
        if (Utils::DocumentBundle::hasBundleExtension(relPath))
            return ProjectExplorer::ProjectEntryKind::Design;
        return ProjectExplorer::ProjectEntryKind::Folder;
    }

    const QString ext = QFileInfo(relPath).suffix().toLower();
    if (ext == "irondesign" || ext == "graphml" || ext == "ironsmith")
        return ProjectExplorer::ProjectEntryKind::Design;

    return ProjectExplorer::ProjectEntryKind::Asset;
}

ProjectExplorer::ProjectEntryList ProjectExplorerDataSource::scanEntries(const QString& rootPath,
                                                                          const Utils::PathPatternMatcher& matcher)
{
    ProjectExplorer::ProjectEntryList entries;

    QDir rootDir(rootPath);
    const QFileInfo rootInfo(rootPath);
    if (!rootInfo.exists() || !rootInfo.isDir())
        return entries;

    QVector<QFileInfo> stack;
    stack.push_back(rootInfo);

    while (!stack.isEmpty()) {
        const QFileInfo currentInfo = stack.takeLast();
        QDir dir(currentInfo.absoluteFilePath());
        const QFileInfoList children = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);
        for (const QFileInfo& fi : children) {
            const QString rel = QDir::fromNativeSeparators(rootDir.relativeFilePath(fi.filePath()));
            if (rel.isEmpty())
                continue;

            const bool isDir = fi.isDir();
            if (matcher.matches(rel, isDir))
                continue;

            ProjectExplorer::ProjectEntry entry;
            entry.path = rel;
            entry.kind = classifyPath(rel, isDir);
            entries.push_back(entry);

            if (isDir && entry.kind == ProjectExplorer::ProjectEntryKind::Folder)
                stack.push_back(fi);
        }
    }

    return entries;
}

QStringList ProjectExplorerDataSource::loadIgnoreFile(const QString& rootPath, const QString& fileName)
{
    QStringList patterns;
    if (rootPath.isEmpty())
        return patterns;

    QFile file(QDir(rootPath).filePath(fileName));
    if (!file.exists())
        return patterns;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return patterns;

    QTextStream stream(&file);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty())
            continue;
        if (line.startsWith('#'))
            continue;
        if (line.startsWith('!')) // negation not supported
            continue;
        patterns.push_back(line);
    }

    return patterns;
}

QStringList ProjectExplorerDataSource::buildIgnorePatterns() const
{
    QStringList patterns = kDefaultIgnorePatterns;
    patterns.append(m_ignorePatterns);

    if (m_useIronIgnore)
        patterns.append(loadIgnoreFile(m_rootPath, QStringLiteral(".ironsmithignore")));

    if (m_useGitIgnore)
        patterns.append(loadIgnoreFile(m_rootPath, QStringLiteral(".gitignore")));

    return patterns;
}

} // namespace ProjectExplorer::Internal
