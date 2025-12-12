// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#include "PluginManager.hpp"
#include "PluginSpec.hpp"
#include "ExtensionSystemGlobal.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QSet>

namespace aiecad {

namespace {

// Build a map from plugin id (lowercased) to PluginSpec*.
QHash<QString, PluginSpec*> buildIdMap(const QList<PluginSpec*> &specs)
{
    QHash<QString, PluginSpec*> map;
    for (PluginSpec *spec : specs) {
        if (!spec)
            continue;
        const QString key = spec->id().toLower();
        if (!key.isEmpty() && !map.contains(key))
            map.insert(key, spec);
    }
    return map;
}

// Compute a topological load order using only required dependencies.
// Disabled or error plugins are excluded from the order.
// Plugins participating in a cycle are marked with an error.
QList<PluginSpec*> computeLoadOrder(const QList<PluginSpec*> &specs)
{
    QList<PluginSpec*> result;

    // 1) Filter enabled, non-error specs
    QList<PluginSpec*> candidates;
    candidates.reserve(specs.size());
    for (PluginSpec *spec : specs) {
        if (!spec)
            continue;
        if (!spec->isEnabled())
            continue;
        if (spec->hasError())
            continue;
        candidates.append(spec);
    }

    if (candidates.isEmpty())
        return result;

    const QHash<QString, PluginSpec*> idMap = buildIdMap(candidates);

    // 2) Build adjacency list and indegree map
    QHash<PluginSpec*, int> indegree;
    QHash<PluginSpec*, QList<PluginSpec*>> adjacency;

    for (PluginSpec *spec : candidates) {
        indegree.insert(spec, 0);
        adjacency.insert(spec, {});
    }

    for (PluginSpec *spec : candidates) {
        const auto &deps = spec->dependencies();
        for (const PluginDependency &dep : deps) {
            if (dep.type != PluginDependency::Type::Required)
                continue;

            PluginSpec *depSpec = idMap.value(dep.id.toLower(), nullptr);
            if (!depSpec)
                continue; // already treated as error in resolveDependencies

            // depSpec -> spec
            adjacency[depSpec].append(spec);
            indegree[spec] += 1;
        }
    }

    // 3) Kahn's algorithm
    QList<PluginSpec*> queue;
    queue.reserve(indegree.size());
    for (auto it = indegree.cbegin(); it != indegree.cend(); ++it) {
        if (it.value() == 0)
            queue.append(it.key());
    }

    while (!queue.isEmpty()) {
        PluginSpec *n = queue.takeFirst();
        result.append(n);

        const QList<PluginSpec*> &neighbors = adjacency.value(n);
        for (PluginSpec *m : neighbors) {
            int &deg = indegree[m];
            deg -= 1;
            if (deg == 0)
                queue.append(m);
        }
    }

    // 4) Detect cycles
    if (result.size() != indegree.size()) {
        // All nodes not in result are part of a cycle or depend on one.
        QSet<PluginSpec*> inResult(result.cbegin(), result.cend());

        for (PluginSpec *spec : candidates) {
            if (!inResult.contains(spec)) {
                QString err = QStringLiteral(
                    "Plugin %1 participates in a dependency cycle.")
                                  .arg(spec->id());
                spec->addError(err);
                qCWarning(aiecadExtensionSystemLog)
                    << "[PluginManager] Dependency cycle involving plugin"
                    << spec->id() << ":" << err;
            }
        }

        // Rebuild a filtered result without newly errored specs.
        QList<PluginSpec*> filtered;
        filtered.reserve(result.size());
        for (PluginSpec *spec : result) {
            if (!spec->hasError())
                filtered.append(spec);
        }
        result = filtered;
    }

    return result;
}

} // namespace

//
// PluginManagerPrivate
//

class PluginManager::PluginManagerPrivate {
public:
    QStringList pluginPaths;
    QList<PluginSpec*> pluginSpecs;  // all discovered specs (including broken)
    QList<PluginSpec*> loadOrder;    // successfully ordered loadable specs

    QList<QObject*> objectPool;

    QReadWriteLock lock;
};

PluginManager::PluginManagerPrivate *PluginManager::d()
{
    static PluginManagerPrivate instance;
    return &instance;
}

//
// Plugin paths
//

void PluginManager::setPluginPaths(const QStringList &paths)
{
    auto *priv = d();
    QWriteLocker locker(&priv->lock);

    priv->pluginPaths = paths;
}

QStringList PluginManager::pluginPaths()
{
    auto *priv = d();
    QReadLocker locker(&priv->lock);

    return priv->pluginPaths;
}

void PluginManager::addPluginPath(const QString &path)
{
    auto *priv = d();
    QWriteLocker locker(&priv->lock);

    if (!priv->pluginPaths.contains(path))
        priv->pluginPaths.append(path);
}

//
// Plugin discovery + loading
//

void PluginManager::loadPlugins(const QStringList &args)
{
    auto *priv = d();

    QList<PluginSpec*> discoveredSpecs;
    QList<PluginSpec*> loadOrder;

    {
        // Critical section: build specs and load order under the lock.
        QWriteLocker locker(&priv->lock);

        // Clear previous state
        for (PluginSpec *spec : priv->pluginSpecs)
            delete spec;
        priv->pluginSpecs.clear();
        priv->loadOrder.clear();

        if (priv->pluginPaths.isEmpty()) {
            qCWarning(aiecadExtensionSystemLog)
                << "[PluginManager] No plugin paths configured.";
            return;
        }

        qCInfo(aiecadExtensionSystemLog)
            << "[PluginManager] Scanning plugin paths:" << priv->pluginPaths;

        // 1. Discover JSON metadata files
        for (const QString &basePath : priv->pluginPaths) {
            QDir baseDir(basePath);
            if (!baseDir.exists()) {
                qCWarning(aiecadExtensionSystemLog)
                    << "[PluginManager] Plugin path does not exist:" << basePath;
                continue;
            }

            QDirIterator it(basePath,
                            QStringList() << QStringLiteral("*.json"),
                            QDir::Files | QDir::Readable,
                            QDirIterator::Subdirectories);

            while (it.hasNext()) {
                const QString jsonFilePath = it.next();
                auto *spec = new PluginSpec;

                QString err;
                if (!spec->readMetaData(jsonFilePath, err)) {
                    qCWarning(aiecadExtensionSystemLog)
                        << "[PluginManager] Failed to read metadata from"
                        << jsonFilePath << ":" << err;
                    delete spec;
                    continue;
                }

                spec->setArguments(args);
                discoveredSpecs.append(spec);

                qCInfo(aiecadExtensionSystemLog)
                    << "[PluginManager] Discovered plugin:"
                    << spec->id() << "from" << jsonFilePath;
            }
        }

        // 2. Resolve dependencies
        for (PluginSpec *spec : discoveredSpecs) {
            QString err;
            if (!spec->resolveDependencies(discoveredSpecs, err)) {
                qCWarning(aiecadExtensionSystemLog)
                    << "[PluginManager] Dependency resolution failed for plugin"
                    << spec->id() << ":" << err;
                // spec stays; its disabled-by-error flag will prevent loading
            }
        }

        // 3. Compute topological load order (enabled, non-error specs)
        loadOrder = computeLoadOrder(discoveredSpecs);

        // 4. Store for later queries + shutdown
        priv->pluginSpecs = discoveredSpecs;
        priv->loadOrder   = loadOrder;
    } // lock released here

    // 5. Load libraries and initialize plugins outside the lock
    for (PluginSpec *spec : loadOrder) {
        if (!spec->isEnabled())
            continue;
        if (spec->hasError())
            continue;

        QString err;
        if (!spec->loadLibrary(err)) {
            qCWarning(aiecadExtensionSystemLog)
                << "[PluginManager] Failed to load library for plugin"
                << spec->id() << ":" << err;
            continue;
        }

        err.clear();
        if (!spec->initializePlugin(err)) {
            qCWarning(aiecadExtensionSystemLog)
                << "[PluginManager] Failed to initialize plugin"
                << spec->id() << ":" << err;
            continue;
        }

        qCInfo(aiecadExtensionSystemLog)
            << "[PluginManager] Loaded and initialized plugin" << spec->id();
    }

    // 6. Broadcast extensionsInitialized(), also outside the lock
    for (PluginSpec *spec : loadOrder) {
        if (!spec->isEnabled())
            continue;
        if (spec->hasError())
            continue;
        if (!spec->pluginInstance())
            continue;

        if (spec->state() == PluginState::Initialized)
            spec->extensionsInitialized();
    }
}

//
// Shutdown
//

void PluginManager::shutdown()
{
    auto *priv = d();
    QWriteLocker locker(&priv->lock);

    // First, clear the object pool (services published by plugins).
    for (QObject *obj : priv->objectPool)
        delete obj;
    priv->objectPool.clear();

    // Then, stop plugins in reverse load order (dependency-safe).
    for (int i = priv->loadOrder.size() - 1; i >= 0; --i) {
        PluginSpec *spec = priv->loadOrder.at(i);
        if (!spec)
            continue;

        if (!spec->isEnabled())
            continue;

        spec->stop();
    }

    // Delete all specs (including those not in loadOrder)
    for (PluginSpec *spec : priv->pluginSpecs)
        delete spec;

    priv->pluginSpecs.clear();
    priv->loadOrder.clear();
}

//
// Plugin spec access
//

QList<PluginSpec*> PluginManager::plugins()
{
    auto *priv = d();
    QReadLocker locker(&priv->lock);

    return priv->pluginSpecs;
}

PluginSpec* PluginManager::pluginById(const QString &id)
{
    auto *priv = d();
    QReadLocker locker(&priv->lock);

    for (PluginSpec *spec : priv->pluginSpecs) {
        if (!spec)
            continue;
        if (spec->id().compare(id, Qt::CaseInsensitive) == 0)
            return spec;
    }

    return nullptr;
}

//
// Object pool
//

void PluginManager::addObject(QObject *obj)
{
    if (!obj)
        return;

    auto *priv = d();
    QWriteLocker locker(&priv->lock);

    if (!priv->objectPool.contains(obj))
        priv->objectPool.append(obj);
}

void PluginManager::removeObject(QObject *obj)
{
    if (!obj)
        return;

    auto *priv = d();
    QWriteLocker locker(&priv->lock);

    priv->objectPool.removeAll(obj);
}

QList<QObject*> PluginManager::allObjects()
{
    auto *priv = d();
    QReadLocker locker(&priv->lock);

    return priv->objectPool;
}

} // namespace aiecad