#include "PluginSpec.hpp"

#include "IPlugin.hpp"
#include "ExtensionSystemGlobal.hpp"

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QLibrary>
#include <QtCore/QDir>

namespace aiecad {

PluginSpec::PluginSpec(QObject *parent)
    : QObject(parent)
{
}

PluginSpec::~PluginSpec() = default;

// ------------------------
// Metadata accessors
// ------------------------

const QString &PluginSpec::id() const
{
    return m_id;
}

const QString &PluginSpec::name() const
{
    return m_name;
}

const QString &PluginSpec::version() const
{
    return m_version;
}

const QString &PluginSpec::description() const
{
    return m_description;
}

const QString &PluginSpec::category() const
{
    return m_category;
}

const QString &PluginSpec::filePath() const
{
    return m_filePath;
}

const QString &PluginSpec::libraryPath() const
{
    return m_libraryPath;
}

const QList<PluginDependency> &PluginSpec::dependencies() const
{
    return m_dependencies;
}

PluginState PluginSpec::state() const
{
    return m_state;
}

// ------------------------
// Enabled / error flags
// ------------------------

bool PluginSpec::isEnabled() const
{
    return m_enabledBySettings && !m_disabledByError;
}

bool PluginSpec::isDisabledBySettings() const
{
    return !m_enabledBySettings;
}

bool PluginSpec::isDisabledByError() const
{
    return m_disabledByError;
}

bool PluginSpec::hasError() const
{
    return !m_errors.isEmpty();
}

const QStringList &PluginSpec::errors() const
{
    return m_errors;
}

// ------------------------
// Plugin instance / arguments
// ------------------------

IPlugin *PluginSpec::pluginInstance() const
{
    return m_pluginInstance;
}

const QStringList &PluginSpec::arguments() const
{
    return m_arguments;
}

// ------------------------
// Plugin ops used by PluginManager
// ------------------------

bool PluginSpec::readMetaData(const QString &filePath, QString &errMsg)
{
    m_errors.clear();
    m_disabledByError = false;

    QFileInfo info(filePath);
    m_filePath = info.absoluteFilePath();

    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errMsg = QStringLiteral("Failed to open plugin metadata file: %1")
                     .arg(m_filePath);
        addError(errMsg);
        return false;
    }

    const QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        errMsg = QStringLiteral("JSON parse error in %1: %2")
                     .arg(m_filePath, parseError.errorString());
        addError(errMsg);
        return false;
    }

    if (!doc.isObject()) {
        errMsg = QStringLiteral("Metadata in %1 is not a JSON object.")
                     .arg(m_filePath);
        addError(errMsg);
        return false;
    }

    const QJsonObject root = doc.object();

    // Required fields: Id, Name, Version
    m_id = root.value(QStringLiteral("Id")).toString().trimmed();
    m_name = root.value(QStringLiteral("Name")).toString().trimmed();
    m_version = root.value(QStringLiteral("Version")).toString().trimmed();

    if (m_id.isEmpty() || m_name.isEmpty() || m_version.isEmpty()) {
        errMsg = QStringLiteral("Metadata in %1 is missing Id, Name, or Version.")
                     .arg(m_filePath);
        addError(errMsg);
        return false;
    }

    // Optional fields
    m_description = root.value(QStringLiteral("Description")).toString().trimmed();
    m_category    = root.value(QStringLiteral("Category")).toString().trimmed();

    // Optional library path override: "LibraryPath"
    // If not provided, we'll derive it later in PluginManager when loading.
    m_libraryPath.clear();
    if (root.contains(QStringLiteral("LibraryPath")))
        m_libraryPath = root.value(QStringLiteral("LibraryPath")).toString().trimmed();

    // Dependencies: array of objects { Id, Version, Type }
    m_dependencies.clear();
    const QJsonValue depsValue = root.value(QStringLiteral("Dependencies"));
    if (depsValue.isArray()) {
        const QJsonArray depsArray = depsValue.toArray();
        for (const auto &v : depsArray) {
            if (!v.isObject())
                continue;

            const QJsonObject depObj = v.toObject();

            PluginDependency dep{};
            dep.id      = depObj.value(QStringLiteral("Id")).toString().trimmed();
            dep.version = depObj.value(QStringLiteral("Version")).toString().trimmed();

            const QString typeStr =
                depObj.value(QStringLiteral("Type")).toString().trimmed().toLower();

            if (typeStr == QLatin1String("optional")) {
                dep.type = PluginDependency::Type::Optional;
            } else if (typeStr == QLatin1String("test")) {
                dep.type = PluginDependency::Type::Test;
            } else {
                dep.type = PluginDependency::Type::Required;
            }

            if (!dep.id.isEmpty())
                m_dependencies.append(dep);
        }
    }

    setState(PluginState::Read);
    return true;
}

bool PluginSpec::resolveDependencies(const QList<PluginSpec *> &allSpecs, QString &errMsg)
{
    if (m_state != PluginState::Read && m_state != PluginState::Resolved) {
        // We only expect to resolve after metadata has been read.
        // Not fatal, but worth logging.
        qCWarning(aiecadExtensionSystemLog)
            << "resolveDependencies() called in unexpected state for plugin"
            << m_id << "state:" << static_cast<int>(m_state);
    }

    // For now, we only check that required dependencies exist.
    // Version checks and dependency cycles can be added later.

    QStringList missingRequired;
    for (const PluginDependency &dep : m_dependencies) {
        if (dep.type != PluginDependency::Type::Required)
            continue;

        bool found = false;
        for (PluginSpec *other : allSpecs) {
            if (!other)
                continue;
            if (other == this)
                continue;
            if (other->id().compare(dep.id, Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        }

        if (!found)
            missingRequired.append(dep.id);
    }

    if (!missingRequired.isEmpty()) {
        errMsg = QStringLiteral("Plugin %1 is missing required dependencies: %2")
                     .arg(m_id, missingRequired.join(QStringLiteral(", ")));
        addError(errMsg);
        return false;
    }

    setState(PluginState::Resolved);
    return true;
}

bool PluginSpec::loadLibrary(QString &errMsg)
{
    if (!isEnabled()) {
        // Disabled by settings or by error; do not load.
        return false;
    }

    if (m_state != PluginState::Resolved && m_state != PluginState::Loaded) {
        qCWarning(aiecadExtensionSystemLog)
            << "loadLibrary() called in unexpected state for plugin"
            << m_id << "state:" << static_cast<int>(m_state);
    }

    QFileInfo metaInfo(m_filePath);
    QDir pluginDir = metaInfo.dir();
    const QString baseName = metaInfo.baseName(); // "Core" from "Core.json"

    // Resolve the actual shared library next to the metadata file.
    // QLibrary::fileName() does NOT add prefixes/suffixes until a load is
    // attempted, so we scan for files that look like libraries and contain
    // the base name (e.g. "libCore.dylib", "Core.dll", etc.).
    const auto resolveLibraryPath = [&]() -> QString {
        QList<QString> candidates;

        // 1) Explicit path from metadata (LibraryPath) if provided.
        if (!m_libraryPath.isEmpty()
            && !m_libraryPath.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
            QFileInfo info(m_libraryPath);
            candidates.append(info.isAbsolute()
                                  ? info.absoluteFilePath()
                                  : pluginDir.absoluteFilePath(m_libraryPath));
        }

        // 2) A few obvious names relative to the metadata directory.
        candidates.append(pluginDir.absoluteFilePath(baseName));
        candidates.append(pluginDir.absoluteFilePath(QStringLiteral("lib") + baseName));

        // 3) Any file in the plugin directory that looks relevant.
        const QFileInfoList files =
            pluginDir.entryInfoList(QDir::Files | QDir::Readable | QDir::NoSymLinks);
        for (const QFileInfo &info : files) {
            if (info.fileName().compare(metaInfo.fileName(), Qt::CaseInsensitive) == 0)
                continue; // skip the metadata json itself

            const bool nameMatches =
                info.fileName().contains(baseName, Qt::CaseInsensitive)
                || info.completeBaseName().contains(baseName, Qt::CaseInsensitive);
            if (!nameMatches)
                continue;

            candidates.append(info.absoluteFilePath());
        }

        for (const QString &candidate : candidates) {
            if (QLibrary::isLibrary(candidate))
                return candidate;
        }

        return {};
    };

    const QString resolvedLibPath = resolveLibraryPath();

    if (resolvedLibPath.isEmpty()) {
        errMsg = QStringLiteral("Could not find plugin library for %1 under: %2")
                     .arg(m_id, pluginDir.absolutePath());
        addError(errMsg);
        return false;
    }

    m_libraryPath = resolvedLibPath;

    if (!m_loader)
        m_loader = std::make_unique<QPluginLoader>(m_libraryPath);
    else
        m_loader->setFileName(m_libraryPath);

    QObject *rawInstance = m_loader->instance();
    if (!rawInstance) {
        errMsg = QStringLiteral("Failed to load plugin library %1: %2")
                     .arg(m_libraryPath, m_loader->errorString());
        addError(errMsg);
        return false;
    }

    auto *plugin = qobject_cast<IPlugin *>(rawInstance);
    if (!plugin) {
        errMsg = QStringLiteral("Library %1 does not export an aiecad::IPlugin instance.")
                     .arg(m_libraryPath);
        addError(errMsg);

        m_loader->unload();
        m_loader.reset();
        return false;
    }

    m_pluginInstance = plugin;
    setState(PluginState::Loaded);
    return true;
}

bool PluginSpec::initializePlugin(QString &errMsg)
{
    if (!isEnabled()) {
        // Plugin is disabled – treat as “successfully skipped”.
        return true;
    }

    if (!m_pluginInstance) {
        errMsg = QStringLiteral("Plugin %1 has no instance to initialize.").arg(m_id);
        addError(errMsg);
        return false;
    }

    if (!m_pluginInstance->initialize(m_arguments, errMsg)) {
        if (!errMsg.isEmpty())
            addError(errMsg);
        else
            addError(QStringLiteral("initialize() failed for plugin %1").arg(m_id));
        return false;
    }

    setState(PluginState::Initialized);
    return true;
}

void PluginSpec::extensionsInitialized()
{
    if (!isEnabled())
        return;

    if (!m_pluginInstance)
        return;

    // Let the plugin know that all others are ready.
    m_pluginInstance->extensionsInitialized();

    // Only move to Running if we were successfully initialized.
    if (m_state == PluginState::Initialized)
        setState(PluginState::Running);
}

void PluginSpec::stop()
{
    if (!m_pluginInstance || !m_loader) {
        // Nothing to do if the plugin was never loaded/instantiated.
        setState(PluginState::Stopped);
        return;
    }

    // For Tier 1, we assume synchronous shutdown. If a plugin returns
    // AsynchronousShutdown, we simply log a warning and still proceed.
    const auto flag = m_pluginInstance->aboutToShutdown();
    if (flag == IPlugin::ShutdownFlag::AsynchronousShutdown) {
        qCWarning(aiecadExtensionSystemLog)
            << "Plugin" << m_id
            << "requested asynchronous shutdown, which is not yet supported. "
               "Treating as synchronous.";
    }

    // Unload the plugin library. QPluginLoader will delete the instance.
    if (!m_loader->unload()) {
        qCWarning(aiecadExtensionSystemLog)
            << "Failed to unload plugin library for" << m_id << ":"
            << m_loader->errorString();
    }

    m_pluginInstance = nullptr;
    m_loader.reset();

    setState(PluginState::Stopped);
}

// ------------------------
// Internal setters
// ------------------------

void PluginSpec::setArguments(const QStringList &args)
{
    m_arguments = args;
}

void PluginSpec::setEnabledBySettings(bool enabled)
{
    m_enabledBySettings = enabled;
}

// ------------------------
// Private helpers
// ------------------------

void PluginSpec::setState(PluginState newState)
{
    if (m_state == newState)
        return;

    m_state = newState;
    emit stateChanged(m_state);
}

void PluginSpec::addError(const QString &msg)
{
    if (msg.isEmpty())
        return;

    m_errors.append(msg);
    m_disabledByError = true;

    qCWarning(aiecadExtensionSystemLog)
        << "PluginSpec error for" << m_id << ":" << msg;
}

} // namespace aiecad
