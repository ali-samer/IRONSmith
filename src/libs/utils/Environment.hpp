// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QStringView>
#include <QtCore/QVariant>

#include <cstddef>
#include <optional>
#include <type_traits>

using namespace Qt::StringLiterals;

namespace Utils {

enum class EnvironmentScope : unsigned char {
    Global,
    Workspace,
    Session
};

struct EnvironmentConfig final {
    QString organizationName;
    QString applicationName;

    QString workspaceRootDir;
    QString globalConfigRootOverride;

    std::size_t maxStateDocumentBytes = 4u * 1024u * 1024u;   // 4 MiB
};

struct EnvironmentPaths final {
    QString globalConfigDir;    // resolved absolute
    QString workspaceConfigDir; // resolved absolute or empty
    QString sessionConfigDir;   // resolved absolute
};

struct DocumentLoadResult final {
    enum class Status : unsigned char {
        Ok,
        NotFound,
        Corrupt
    };

    Status status = Status::NotFound;
    QJsonObject object;
    bool fromBackup = false;
    QString error;
};

struct DocumentSaveResult final {
    bool ok = false;
    QString error;
};

template <typename PersistencePolicy>
class BasicEnvironment final {
public:
    using Policy = PersistencePolicy;
    using SettingsHandle = typename Policy::SettingsHandle;

    explicit BasicEnvironment(EnvironmentConfig config, Policy policy = Policy{})
        : m_config(std::move(config))
        , m_policy(std::move(policy))
        , m_paths(m_policy.resolvePaths(m_config))
    {}

    const EnvironmentConfig& config() const noexcept { return m_config; }
    const EnvironmentPaths& paths()  const noexcept { return m_paths; }
    const Policy& policy() const noexcept { return m_policy; }

    QVariant setting(EnvironmentScope scope, QStringView key, const QVariant& def = {}) const
    {
        auto h = m_policy.openSettings(scope, m_paths);
        return m_policy.settingsValue(h, key, def);
    }

    void setSetting(EnvironmentScope scope, QStringView key, const QVariant& value)
    {
        auto h = m_policy.openSettings(scope, m_paths);
        m_policy.setSettingsValue(h, key, value);
        m_policy.syncSettings(h);
    }

    void removeSetting(EnvironmentScope scope, QStringView key)
    {
        auto h = m_policy.openSettings(scope, m_paths);
        m_policy.removeSettingsKey(h, key);
        m_policy.syncSettings(h);
    }

    bool hasSetting(EnvironmentScope scope, QStringView key) const
    {
        auto h = m_policy.openSettings(scope, m_paths);
        return m_policy.settingsContains(h, key);
    }

    QString themeId(EnvironmentScope scope = EnvironmentScope::Global) const
    {
        return setting(scope, u"ui/themeId"_s, QString{}).toString();
    }

    void setThemeId(const QString& themeId, EnvironmentScope scope = EnvironmentScope::Global)
    {
        setSetting(scope, u"ui/themeId"_s, themeId);
    }

    ///
    // State documents

    DocumentLoadResult loadState(EnvironmentScope scope, QStringView name) const
    {
        DocumentLoadResult result;

        QString ensureErr;
        if (!m_policy.ensureScopeStorage(scope, m_paths, &ensureErr)) {
            result.status = DocumentLoadResult::Status::Corrupt;
            result.error = ensureErr.isEmpty() ? QStringLiteral("Failed to ensure storage") : ensureErr;
            return result;
        }

        {
            QByteArray bytes;
            QString err;
            if (m_policy.readStateBytes(scope, m_paths, name, /*useBackup=*/false, &bytes, &err)) {
                auto parsed = parseJson(bytes, /*fromBackup=*/false);
                if (parsed.status == DocumentLoadResult::Status::Ok)
                    return parsed;

                result.error = parsed.error;
            } else {
                if (!err.isEmpty())
                    result.error = err;
            }
        }

        {
            QByteArray bytes;
            QString err;
            if (m_policy.readStateBytes(scope, m_paths, name, /*useBackup=*/true, &bytes, &err)) {
                auto parsed = parseJson(bytes, /*fromBackup=*/true);
                if (parsed.status == DocumentLoadResult::Status::Ok)
                    return parsed;

                result.status = DocumentLoadResult::Status::Corrupt;
                result.error = !parsed.error.isEmpty() ? parsed.error
                                                      : (!err.isEmpty() ? err
                                                                        : QStringLiteral("Backup state document is invalid."));
                return result;
            }

            if (!err.isEmpty()) {
                result.status = DocumentLoadResult::Status::Corrupt;
                result.error = err;
                return result;
            }
        }

        if (!result.error.isEmpty()) {
            result.status = DocumentLoadResult::Status::Corrupt;
            return result;
        }

        result.status = DocumentLoadResult::Status::NotFound;
        return result;
    }

    DocumentSaveResult saveState(EnvironmentScope scope, QStringView name, const QJsonObject& object) const
    {
        DocumentSaveResult out;

        QString ensureErr;
        if (!m_policy.ensureScopeStorage(scope, m_paths, &ensureErr)) {
            out.ok = false;
            out.error = ensureErr.isEmpty() ? QStringLiteral("Failed to ensure storage.") : ensureErr;
            return out;
        }

        const QJsonDocument doc(object);
        const QByteArray bytes = doc.toJson(QJsonDocument::Compact);

        if (bytes.size() < 0 || static_cast<std::size_t>(bytes.size()) > m_config.maxStateDocumentBytes) {
            out.ok = false;
            out.error = QStringLiteral("State document exceeds maxStateDocumentBytes (limit: %1).").arg(m_config.maxStateDocumentBytes);
            return out;
        }

        QString err;
        out.ok = m_policy.writeStateBytesAtomic(scope, m_paths, name, bytes, &err);
        out.error = err;
        return out;
    }

    bool removeState(EnvironmentScope scope, QStringView name, bool removeBackup = true, QString* error = nullptr) const
    {
        QString err;
        const bool ok = m_policy.removeState(scope, m_paths, name, removeBackup, &err);
        if (error) *error = err;
        return ok;
    }

private:
    DocumentLoadResult parseJson(const QByteArray& bytes, bool fromBackup) const
    {
        DocumentLoadResult result;

        if (bytes.size() < 0 || static_cast<std::size_t>(bytes.size()) > m_config.maxStateDocumentBytes) {
            result.status = DocumentLoadResult::Status::Corrupt;
            result.fromBackup = fromBackup;
            result.error = QStringLiteral("State document exceeds maxStateDocumentBytes (limit: %1).").arg(m_config.maxStateDocumentBytes);
            return result;
        }

        QJsonParseError pe{};
        const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
        if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
            result.status = DocumentLoadResult::Status::Corrupt;
            result.fromBackup = fromBackup;
            result.error = QStringLiteral("Invalid JSON state document.");
            return result;
        }

        result.status = DocumentLoadResult::Status::Ok;
        result.fromBackup = fromBackup;
        result.object = doc.object();
        return result;
    }

private:
    EnvironmentConfig m_config;
    Policy m_policy;
    EnvironmentPaths m_paths;
};

} // namespace Utils
