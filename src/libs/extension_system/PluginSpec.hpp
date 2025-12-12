// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#pragma once

#include "ExtensionSystemGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QPluginLoader>

#include <memory>

namespace aiecad {
class IPlugin;

// Lifecycle state of a single plugin specification.
// The state is driven by the PluginManager as it reads metadata,
// resolves dependencies, loads the library, and initializes the plugin.
enum class PluginState {
	Invalid,     // No metadata loaded; spec is unusable.
	Read,        // Metadata successfully parsed from JSON.
	Resolved,    // Dependencies resolved against other specs.
	Loaded,      // Shared library loaded; plugin instance created.
	Initialized, // initialize() succeeded.
	Running,     // extensionsInitialized() has been called.
	Stopped      // Plugin has been shut down.
};

// Describes a single dependency as declared in plugin metadata.
struct AIECAD_EXTENSION_SYSTEM_EXPORT PluginDependency {
	enum class Type {
		Required, // Plugin must be present and loadable.
		Optional, // Plugin is optinal; absense is not fatal.
		Test,     // Only needed when running tests.
	};

	QString id;
	QString version;
	Type    type = Type::Required;

	bool operator==(const PluginDependency &other) const = default;
};

// PluginSpec holds the metadata, dependency information, state, and
// plugin instance pointer for a single plugin. It does NOT load itself;
// PluginManager controls its lifecycle.
class AIECAD_EXTENSION_SYSTEM_EXPORT PluginSpec : public QObject {
	Q_OBJECT

public:
	explicit PluginSpec(QObject *parent = nullptr);
	~PluginSpec() override;

	const QString& id() const;
	const QString& name() const;
	const QString& version() const;
	const QString& description() const;
	const QString& category() const;

	const QString& filePath() const;    // Path to JSON metadata file
	const QString& libraryPath() const; // Path to shared library

	const QList<PluginDependency>& dependencies() const;

	PluginState state() const;

	bool isEnabled() const;
	bool isDisabledBySettings() const;
	bool isDisabledByError() const;

	bool               hasError() const;
	const QStringList& errors() const;

	IPlugin* pluginInstance() const;

	// Arguments passed to the plugin (from CLI/config) during initialize().
	const QStringList& arguments() const;

	//
	// Plugin ops used by PluginManager
	bool readMetaData(const QString &filePath, QString &errMsg);
	bool resolveDependencies(const QList<PluginSpec*> &allSpecs, QString &errMsg);

	bool loadLibrary(QString &errMsg);
	bool initializePlugin(QString &errMsg);

	void extensionsInitialized();

	void stop();

	void setArguments(const QStringList &args);
	void setEnabledBySettings(bool enabled);
	void addError(const QString &msg);

signals:
	void stateChanged(aiecad::PluginState newState);

private:
	void setState(PluginState newState);

private:
	QString m_id;
	QString m_name;
	QString m_version;
	QString m_description;
	QString m_category;

	QString m_filePath;
	QString m_libraryPath;

	QList<PluginDependency> m_dependencies;

	QStringList m_arguments;
	QStringList m_errors;

	IPlugin *m_pluginInstance{ nullptr };

	PluginState m_state{ PluginState::Invalid };

	bool m_enabledBySettings{ true };
	bool m_disabledByError{ false };

	std::unique_ptr<QPluginLoader> m_loader;
};
} // namespace aiecad
