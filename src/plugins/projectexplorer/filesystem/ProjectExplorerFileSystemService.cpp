#include "projectexplorer/filesystem/ProjectExplorerFileSystemService.hpp"

#include <utils/PathUtils.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QSaveFile>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>

namespace ProjectExplorer::Internal {

ProjectExplorerFileSystemService::ProjectExplorerFileSystemService(QObject* parent)
    : QObject(parent)
{
}

QString ProjectExplorerFileSystemService::rootPath() const
{
    return m_rootPath;
}

void ProjectExplorerFileSystemService::setRootPath(const QString& path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned == m_rootPath)
        return;
    m_rootPath = cleaned;
    emit rootPathChanged(m_rootPath);
}

Utils::Result ProjectExplorerFileSystemService::openPath(const QString& relPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::Open, relPath, error);
        return Utils::Result::failure(error);
    }

    const QString abs = absolutePathFor(relPath);
    if (abs.isEmpty())
        return Utils::Result::failure(QStringLiteral("Path is empty."));

    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(abs));
    if (!ok) {
        const QString msg = QStringLiteral("Failed to open path: %1").arg(abs);
        emit operationFailed(Operation::Open, relPath, msg);
        return Utils::Result::failure(msg);
    }

    emit operationCompleted(Operation::Open, relPath, QString());
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::revealPath(const QString& relPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::Reveal, relPath, error);
        return Utils::Result::failure(error);
    }

    const QString abs = absolutePathFor(relPath);
    if (abs.isEmpty())
        return Utils::Result::failure(QStringLiteral("Path is empty."));

#if defined(Q_OS_MAC)
    const QStringList args = { QStringLiteral("-R"), abs };
    const int code = QProcess::execute(QStringLiteral("open"), args);
    if (code != 0) {
        const QString msg = QStringLiteral("Failed to reveal in Finder: %1").arg(abs);
        emit operationFailed(Operation::Reveal, relPath, msg);
        return Utils::Result::failure(msg);
    }
#elif defined(Q_OS_WIN)
    const QStringList args = { QStringLiteral("/select,"), QDir::toNativeSeparators(abs) };
    if (!QProcess::startDetached(QStringLiteral("explorer"), args)) {
        const QString msg = QStringLiteral("Failed to reveal in Explorer: %1").arg(abs);
        emit operationFailed(Operation::Reveal, relPath, msg);
        return Utils::Result::failure(msg);
    }
#else
    const QString dirPath = QFileInfo(abs).absolutePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(dirPath))) {
        const QString msg = QStringLiteral("Failed to reveal in file manager: %1").arg(abs);
        emit operationFailed(Operation::Reveal, relPath, msg);
        return Utils::Result::failure(msg);
    }
#endif

    emit operationCompleted(Operation::Reveal, relPath, QString());
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::renamePath(const QString& relPath,
                                                           const QString& newName,
                                                           QString* outNewRelPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::Rename, relPath, error);
        return Utils::Result::failure(error);
    }

    const QString abs = absolutePathFor(relPath);
    QFileInfo fi(abs);
    if (!fi.exists()) {
        const QString msg = QStringLiteral("Path does not exist: %1").arg(abs);
        emit operationFailed(Operation::Rename, relPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString sanitized = Utils::PathUtils::sanitizeFileName(newName);
    if (sanitized.isEmpty()) {
        const QString msg = QStringLiteral("Name cannot be empty.");
        emit operationFailed(Operation::Rename, relPath, msg);
        return Utils::Result::failure(msg);
    }

    if (sanitized == fi.fileName())
        return Utils::Result::success();

    const QString newAbs = QDir(fi.absolutePath()).filePath(sanitized);
    if (QFileInfo::exists(newAbs)) {
        const QString msg = QStringLiteral("A file or folder with that name already exists.");
        emit operationFailed(Operation::Rename, relPath, msg);
        return Utils::Result::failure(msg);
    }

    QDir parentDir(fi.absolutePath());
    const bool ok = parentDir.rename(fi.fileName(), sanitized);
    if (!ok) {
        const QString msg = QStringLiteral("Failed to rename '%1'.").arg(fi.fileName());
        emit operationFailed(Operation::Rename, relPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString newRel = QDir(m_rootPath).relativeFilePath(newAbs);
    if (outNewRelPath)
        *outNewRelPath = newRel;

    emit operationCompleted(Operation::Rename, relPath, newRel);
    emit refreshRequested();
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::removePath(const QString& relPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::Delete, relPath, error);
        return Utils::Result::failure(error);
    }

    const QString abs = absolutePathFor(relPath);
    QFileInfo fi(abs);
    if (!fi.exists()) {
        const QString msg = QStringLiteral("Path does not exist: %1").arg(abs);
        emit operationFailed(Operation::Delete, relPath, msg);
        return Utils::Result::failure(msg);
    }

    bool ok = false;
    if (fi.isDir()) {
        QDir dir(abs);
        ok = dir.removeRecursively();
    } else {
        ok = QFile::remove(abs);
    }

    if (!ok) {
        const QString msg = QStringLiteral("Failed to delete '%1'.").arg(fi.fileName());
        emit operationFailed(Operation::Delete, relPath, msg);
        return Utils::Result::failure(msg);
    }

    emit operationCompleted(Operation::Delete, relPath, QString());
    emit refreshRequested();
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::duplicatePath(const QString& relPath, QString* outNewRelPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::Duplicate, relPath, error);
        return Utils::Result::failure(error);
    }

    const QString abs = absolutePathFor(relPath);
    QFileInfo fi(abs);
    if (!fi.exists()) {
        const QString msg = QStringLiteral("Path does not exist: %1").arg(abs);
        emit operationFailed(Operation::Duplicate, relPath, msg);
        return Utils::Result::failure(msg);
    }

    const QDir dir(fi.absolutePath());
    const QString newName = duplicateName(dir, fi.fileName());
    if (newName.isEmpty()) {
        const QString msg = QStringLiteral("Failed to generate duplicate name for '%1'.").arg(fi.fileName());
        emit operationFailed(Operation::Duplicate, relPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString destAbs = dir.filePath(newName);
    Utils::Result result = fi.isDir() ? copyRecursively(abs, destAbs)
                                      : (QFile::copy(abs, destAbs) ? Utils::Result::success()
                                                                  : Utils::Result::failure(QStringLiteral("Copy failed.")));
    if (!result.ok) {
        const QString msg = result.errors.isEmpty() ? QStringLiteral("Copy failed.") : result.errors.join("\n");
        emit operationFailed(Operation::Duplicate, relPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString newRel = QDir(m_rootPath).relativeFilePath(destAbs);
    if (outNewRelPath)
        *outNewRelPath = newRel;

    emit operationCompleted(Operation::Duplicate, relPath, newRel);
    emit refreshRequested();
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::createFolder(const QString& parentRelPath,
                                                             const QString& name,
                                                             QString* outNewRelPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::NewFolder, parentRelPath, error);
        return Utils::Result::failure(error);
    }

    const QString targetDir = resolveTargetDirectory(parentRelPath);
    if (targetDir.isEmpty()) {
        const QString msg = QStringLiteral("Target directory is invalid.");
        emit operationFailed(Operation::NewFolder, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString sanitized = Utils::PathUtils::sanitizeFileName(name);
    if (sanitized.isEmpty()) {
        const QString msg = QStringLiteral("Folder name cannot be empty.");
        emit operationFailed(Operation::NewFolder, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    QDir dir(targetDir);
    const QString newName = uniqueChildName(dir, sanitized, QString());
    if (newName.isEmpty()) {
        const QString msg = QStringLiteral("Unable to create folder with that name.");
        emit operationFailed(Operation::NewFolder, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    if (!dir.mkdir(newName)) {
        const QString msg = QStringLiteral("Failed to create folder '%1'.").arg(newName);
        emit operationFailed(Operation::NewFolder, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString newAbs = dir.filePath(newName);
    const QString newRel = QDir(m_rootPath).relativeFilePath(newAbs);
    if (outNewRelPath)
        *outNewRelPath = newRel;

    emit operationCompleted(Operation::NewFolder, parentRelPath, newRel);
    emit refreshRequested();
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::createDesign(const QString& parentRelPath,
                                                             const QString& name,
                                                             QString* outNewRelPath)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::NewDesign, parentRelPath, error);
        return Utils::Result::failure(error);
    }

    const QString targetDir = resolveTargetDirectory(parentRelPath);
    if (targetDir.isEmpty()) {
        const QString msg = QStringLiteral("Target directory is invalid.");
        emit operationFailed(Operation::NewDesign, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString sanitized = Utils::PathUtils::sanitizeFileName(name);
    if (sanitized.isEmpty()) {
        const QString msg = QStringLiteral("Design name cannot be empty.");
        emit operationFailed(Operation::NewDesign, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString designName = Utils::PathUtils::ensureExtension(sanitized, QStringLiteral("irondesign"));

    QDir dir(targetDir);
    const QFileInfo fi(designName);
    const QString base = fi.completeBaseName();
    const QString ext = fi.completeSuffix();
    const QString unique = uniqueChildName(dir, base, ext);
    if (unique.isEmpty()) {
        const QString msg = QStringLiteral("Unable to create design with that name.");
        emit operationFailed(Operation::NewDesign, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString abs = dir.filePath(unique);
    QSaveFile file(abs);
    if (!file.open(QIODevice::WriteOnly)) {
        const QString msg = QStringLiteral("Failed to create design '%1'.").arg(unique);
        emit operationFailed(Operation::NewDesign, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }
    file.write("{}\n");
    if (!file.commit()) {
        const QString msg = QStringLiteral("Failed to save design '%1'.").arg(unique);
        emit operationFailed(Operation::NewDesign, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    const QString newRel = QDir(m_rootPath).relativeFilePath(abs);
    if (outNewRelPath)
        *outNewRelPath = newRel;

    emit operationCompleted(Operation::NewDesign, parentRelPath, newRel);
    emit refreshRequested();
    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::importAssets(const QString& parentRelPath,
                                                             const QStringList& sourcePaths,
                                                             QStringList* outNewRelPaths)
{
    QString error;
    if (!ensureRoot(&error)) {
        emit operationFailed(Operation::ImportAsset, parentRelPath, error);
        return Utils::Result::failure(error);
    }

    if (sourcePaths.isEmpty())
        return Utils::Result::success();

    const QString targetDir = resolveTargetDirectory(parentRelPath);
    if (targetDir.isEmpty()) {
        const QString msg = QStringLiteral("Target directory is invalid.");
        emit operationFailed(Operation::ImportAsset, parentRelPath, msg);
        return Utils::Result::failure(msg);
    }

    QDir dir(targetDir);
    QStringList imported;

    for (const QString& source : sourcePaths) {
        QFileInfo fi(source);
        if (!fi.exists())
            continue;

        const QString unique = uniqueChildName(dir, fi.completeBaseName(), fi.completeSuffix());
        if (unique.isEmpty())
            continue;

        const QString destAbs = dir.filePath(unique);
        Utils::Result result = fi.isDir() ? copyRecursively(source, destAbs)
                                          : (QFile::copy(source, destAbs) ? Utils::Result::success()
                                                                         : Utils::Result::failure(QStringLiteral("Copy failed.")));
        if (result.ok) {
            const QString newRel = QDir(m_rootPath).relativeFilePath(destAbs);
            imported.push_back(newRel);
            emit operationCompleted(Operation::ImportAsset, parentRelPath, newRel);
        }
    }

    if (outNewRelPaths)
        *outNewRelPaths = imported;

    emit refreshRequested();
    return Utils::Result::success();
}

QString ProjectExplorerFileSystemService::absolutePathFor(const QString& relPath) const
{
    if (m_rootPath.isEmpty())
        return {};
    if (relPath.isEmpty())
        return m_rootPath;
    return QDir(m_rootPath).filePath(relPath);
}

QString ProjectExplorerFileSystemService::resolveTargetDirectory(const QString& relPath) const
{
    const QString abs = absolutePathFor(relPath);
    if (abs.isEmpty())
        return {};

    QFileInfo fi(abs);
    if (fi.exists() && fi.isDir())
        return fi.absoluteFilePath();

    if (fi.exists())
        return fi.absolutePath();

    return abs;
}

QString ProjectExplorerFileSystemService::uniqueChildName(const QDir& dir,
                                                          const QString& baseName,
                                                          const QString& ext) const
{
    const QString trimmedBase = baseName.trimmed();
    if (trimmedBase.isEmpty())
        return {};

    const QString suffix = ext.isEmpty() ? QString() : QStringLiteral(".%1").arg(ext);
    const QString candidate = trimmedBase + suffix;
    if (!dir.exists(candidate))
        return candidate;

    for (int i = 1; i < 1000; ++i) {
        const QString indexed = QStringLiteral("%1 (%2)%3").arg(trimmedBase).arg(i).arg(suffix);
        if (!dir.exists(indexed))
            return indexed;
    }

    return {};
}

QString ProjectExplorerFileSystemService::duplicateName(const QDir& dir, const QString& fileName) const
{
    const QFileInfo fi(fileName);
    const QString base = fi.completeBaseName();
    const QString ext = fi.completeSuffix();
    const QString copyBase = QStringLiteral("%1 copy").arg(base);

    QString candidate = uniqueChildName(dir, copyBase, ext);
    if (!candidate.isEmpty())
        return candidate;

    return uniqueChildName(dir, base, ext);
}

Utils::Result ProjectExplorerFileSystemService::copyRecursively(const QString& source, const QString& dest)
{
    QFileInfo srcInfo(source);
    if (!srcInfo.exists())
        return Utils::Result::failure(QStringLiteral("Source does not exist."));

    if (srcInfo.isDir()) {
        QDir destDir(dest);
        if (!destDir.exists() && !destDir.mkpath(QStringLiteral(".")))
            return Utils::Result::failure(QStringLiteral("Failed to create directory '%1'.").arg(dest));

        QDir srcDir(source);
        const QFileInfoList entries = srcDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo& entry : entries) {
            const QString srcPath = entry.absoluteFilePath();
            const QString destPath = destDir.filePath(entry.fileName());
            const Utils::Result result = copyRecursively(srcPath, destPath);
            if (!result.ok)
                return result;
        }
        return Utils::Result::success();
    }

    if (!QFile::copy(source, dest))
        return Utils::Result::failure(QStringLiteral("Failed to copy file '%1'.").arg(source));

    return Utils::Result::success();
}

Utils::Result ProjectExplorerFileSystemService::ensureRoot(QString* error) const
{
    if (!m_rootPath.isEmpty())
        return Utils::Result::success();
    if (error)
        *error = QStringLiteral("Root path is not set.");
    return Utils::Result::failure(QStringLiteral("Root path is not set."));
}

} // namespace ProjectExplorer::Internal
