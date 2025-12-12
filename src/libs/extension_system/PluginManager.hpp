// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#pragma once

#include "ExtensionSystemGlobal.hpp"
#include "PluginSpec.hpp"

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QReadWriteLock>

namespace aiecad {

class AIECAD_EXTENSION_SYSTEM_EXPORT PluginManager {
public:
	PluginManager() = delete;

	static void        setPluginPaths(const QStringList &paths);
	static QStringList pluginPaths();

	static void addPluginPath(const QString &path);

	static void loadPlugins(const QStringList &args = { });

	// Shutdown all plugins in reverse dependency order.
	// For now we treat all shutdown as synchronous; asynchronous shutdown
	// will be wired later following Qt Creator semantics.
	static void shutdown();

	// Returns all known PluginSpecs (after metadata scan).
	static QList<PluginSpec*> plugins();

	// Find a plugin by ID (case-insensitive match).
	static PluginSpec* pluginById(const QString &id);

	// Add/remove QObject-derived service objects. Plugins typically call this
	// during initialize() to publish their services.
	static void addObject(QObject *obj);
	static void removeObject(QObject *obj);

	static QList<QObject*> allObjects();

	template < class T >
	static T* getObject() {
		const QList<QObject*> objects = allObjects();
		for (QObject *obj : objects) {
			if (auto casted = qobject_cast<T*>(obj))
				return casted;
		}
		return nullptr;
	}

	template < class T >
	static QList<T*> getObjects() {
		QList<T*> results;

		const QList<QObject*> objects = allObjects();
		for (QObject *obj : objects) {
			if (auto casted = qobject_cast<T*>(obj))
				results.append(casted);
		}
		return results;
	}

private:
	class PluginManagerPrivate;
	static PluginManagerPrivate* d();
};
} // namespace aiecad