// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/EnvironmentQtPolicy.hpp"

#include <QtCore/QCryptographicHash>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QStandardPaths>

namespace Utils {

static QString stableWorkspaceKey(const QString& workspaceRootDir)
{
    if (workspaceRootDir.isEmpty())
        return QStringLiteral("no-workspace");

    const QByteArray hash = QCryptographicHash::hash(workspaceRootDir.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(hash);
}

EnvironmentPaths QtEnvironmentPersistencePolicy::resolvePaths(const EnvironmentConfig& cfg) const
{
    EnvironmentPaths out;

    const QString appCfg =
        !cfg.globalConfigRootOverride.isEmpty()
            ? cfg.globalConfigRootOverride
            : QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    const QString globalBase = QDir(appCfg).filePath(cfg.applicationName.isEmpty()
                                                        ? QStringLiteral("IRONSmith")
                                                        : cfg.applicationName);
    out.globalConfigDir = QDir(globalBase).absolutePath();

    if (!cfg.workspaceRootDir.isEmpty()) {
        out.workspaceConfigDir = QDir(cfg.workspaceRootDir).filePath(QStringLiteral(".ironsmith"));
        out.workspaceConfigDir = QDir(out.workspaceConfigDir).absolutePath();
    }

    const QString sessKey = stableWorkspaceKey(cfg.workspaceRootDir);
    out.sessionConfigDir = QDir(out.globalConfigDir).filePath(QStringLiteral("sessions/%1").arg(sessKey));
    out.sessionConfigDir = QDir(out.sessionConfigDir).absolutePath();

    return out;
}

QString QtEnvironmentPersistencePolicy::settingsFilePath(EnvironmentScope scope, const EnvironmentPaths& paths) const
{
    switch (scope) {
    case EnvironmentScope::Global:
        return QDir(paths.globalConfigDir).filePath(QStringLiteral("global.ini"));
    case EnvironmentScope::Workspace:
        return QDir(paths.workspaceConfigDir).filePath(QStringLiteral("workspace.ini"));
    case EnvironmentScope::Session:
        return QDir(paths.sessionConfigDir).filePath(QStringLiteral("session.ini"));
    }
    return QDir(paths.globalConfigDir).filePath(QStringLiteral("global.ini"));
}

QtEnvironmentPersistencePolicy::SettingsHandle
QtEnvironmentPersistencePolicy::openSettings(EnvironmentScope scope, const EnvironmentPaths& paths) const
{
    auto h = SettingsHandle{};
    const QString path = settingsFilePath(scope, paths);
    h.settings = std::make_unique<QSettings>(path, QSettings::IniFormat);
    h.settings->setFallbacksEnabled(false);
    return h;
}

QVariant QtEnvironmentPersistencePolicy::settingsValue(const SettingsHandle& h, QStringView key, const QVariant& def) const
{
    return h.settings ? h.settings->value(key.toString(), def) : def;
}

void QtEnvironmentPersistencePolicy::setSettingsValue(SettingsHandle& h, QStringView key, const QVariant& value) const
{
    if (!h.settings) return;
    h.settings->setValue(key.toString(), value);
}

void QtEnvironmentPersistencePolicy::removeSettingsKey(SettingsHandle& h, QStringView key) const
{
    if (!h.settings) return;
    h.settings->remove(key.toString());
}

bool QtEnvironmentPersistencePolicy::settingsContains(const SettingsHandle& h, QStringView key) const
{
    return h.settings ? h.settings->contains(key.toString()) : false;
}

void QtEnvironmentPersistencePolicy::syncSettings(SettingsHandle& h) const
{
    if (!h.settings) return;
    h.settings->sync();
}

bool QtEnvironmentPersistencePolicy::ensureScopeStorage(EnvironmentScope scope, const EnvironmentPaths& paths, QString* error) const
{
    auto ensureDir = [&](const QString& dir) -> bool {
        if (dir.isEmpty()) {
            if (error) *error = QStringLiteral("Scope storage directory is empty.");
            return false;
        }
        QDir d(dir);
        if (d.exists())
            return true;
        if (!d.mkpath(QStringLiteral("."))) {
            if (error) *error = QStringLiteral("Failed to create directory: %1").arg(dir);
            return false;
        }
        return true;
    };

    switch (scope) {
    case EnvironmentScope::Global:
        return ensureDir(paths.globalConfigDir);
    case EnvironmentScope::Workspace:
        return ensureDir(paths.workspaceConfigDir);
    case EnvironmentScope::Session:
        return ensureDir(paths.sessionConfigDir);
    }
    return ensureDir(paths.globalConfigDir);
}

QString QtEnvironmentPersistencePolicy::stateFilePath(EnvironmentScope scope,
                                                     const EnvironmentPaths& paths,
                                                     QStringView name,
                                                     bool backup) const
{
    const QString base =
        (scope == EnvironmentScope::Global)   ? paths.globalConfigDir
      : (scope == EnvironmentScope::Workspace) ? paths.workspaceConfigDir
                                              : paths.sessionConfigDir;

    const QString file = backup
        ? QStringLiteral("%1.json.bak").arg(name.toString())
        : QStringLiteral("%1.json").arg(name.toString());

    return QDir(base).filePath(QStringLiteral("state/%1").arg(file));
}

bool QtEnvironmentPersistencePolicy::readStateBytes(EnvironmentScope scope, const EnvironmentPaths& paths,
                                                    QStringView name, bool useBackup,
                                                    QByteArray* out, QString* error) const
{
    if (out) out->clear();

    const QString path = stateFilePath(scope, paths, name, useBackup);
    QFile f(path);
    if (!f.exists()) {
        // not found is not an error
        if (error) error->clear();
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("Failed to open state file: %1").arg(path);
        return false;
    }
    const QByteArray bytes = f.readAll();
    if (out) *out = bytes;
    if (error) error->clear();
    return true;
}

bool QtEnvironmentPersistencePolicy::writeStateBytesAtomic(EnvironmentScope scope, const EnvironmentPaths& paths,
                                                           QStringView name, const QByteArray& bytes,
                                                           QString* error) const
{
    const QString primary = stateFilePath(scope, paths, name, /*backup=*/false);
    const QString backup  = stateFilePath(scope, paths, name, /*backup=*/true);

    {
        const QFileInfo fi(primary);
        QDir dir(fi.absolutePath());
        if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
            if (error) *error = QStringLiteral("Failed to create directory: %1").arg(fi.absolutePath());
            return false;
        }
    }

    if (QFile::exists(primary)) {
        QFile::remove(backup);
        QFile::copy(primary, backup);
    }

    QSaveFile sf(primary);
    sf.setDirectWriteFallback(true);
    if (!sf.open(QIODevice::WriteOnly)) {
        if (error) *error = QStringLiteral("Failed to open state file for write: %1").arg(primary);
        return false;
    }

    const qint64 written = sf.write(bytes);
    if (written != bytes.size()) {
        sf.cancelWriting();
        if (error) *error = QStringLiteral("Failed to write complete state document: %1").arg(primary);
        return false;
    }

    if (!sf.commit()) {
        if (error) *error = QStringLiteral("Failed to commit state document: %1").arg(primary);
        return false;
    }

    if (error) error->clear();
    return true;
}

bool QtEnvironmentPersistencePolicy::removeState(EnvironmentScope scope, const EnvironmentPaths& paths,
                                                QStringView name, bool removeBackup,
                                                QString* error) const
{
    const QString primary = stateFilePath(scope, paths, name, /*backup=*/false);
    const QString backup  = stateFilePath(scope, paths, name, /*backup=*/true);

    bool ok = true;
    if (QFile::exists(primary))
        ok = QFile::remove(primary) && ok;
    if (removeBackup && QFile::exists(backup))
        ok = QFile::remove(backup) && ok;

    if (!ok && error)
        *error = QStringLiteral("Failed to remove one or more state files for '%1'.").arg(name.toString());
    else if (error)
        error->clear();

    return ok;
}

} // namespace Utils
