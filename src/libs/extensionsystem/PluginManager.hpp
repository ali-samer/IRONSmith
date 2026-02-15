// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QHash>
#include <QtCore/QSet>

#include <filesystem>

#include "PluginSpec.hpp"

class QPluginLoader;

namespace ExtensionSystem {

class PluginManager final : public QObject
{
	Q_OBJECT

public:
	static PluginManager& instance();

	static void setPluginPaths(const std::vector<std::filesystem::path>& paths);

	static PluginSpec* specById(const char* id);
	static PluginSpec* specById(const QString& id);

	static void registerPlugin(PluginSpec spec);

	static bool registerPlugins(const std::vector<std::filesystem::path>& pluginFiles);
	static bool registerPlugins(const QStringList& pluginFiles);

	static bool loadPlugins(const QStringList& arguments = {});

	static void addObject(QObject* obj);
	static void removeObject(QObject* obj);

	static QObject* getObject(const QString& objectName);

	template <class T>
	static T* getObject()
	{
		for (QObject* o : instance().m_objects) {
			if (auto* casted = qobject_cast<T*>(o))
				return casted;
		}
		return nullptr;
	}

	static QStringList lastErrors();

	~PluginManager() override;

private:
	explicit PluginManager(QObject* parent = nullptr);

	bool validateGraph(QStringList& errors) const;
	bool computeLoadOrder(QVector<QString>& order, QStringList& errors) const;
	void clearRegistrationState();

	bool findCycle(QStringList& cycleOut) const;

	static bool isValidId(const QString& id);

private:
	QVector<QString> m_pluginPaths;

	QHash<QString, PluginSpec> m_specs;
	QVector<QString> m_loadOrder;
	QStringList m_lastErrors;

	QHash<QString, ::QPluginLoader*> m_loadersById;
	QSet<IPlugin*> m_loaderOwnedPlugins;

	QVector<QObject*> m_objects;
	QVector<IPlugin*> m_plugins;
};

} // namespace ExtensionSystem
