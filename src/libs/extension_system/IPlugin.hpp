// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#pragma once

#include "ExtensionSystemGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace aiecad {

// Base class for all AIECAD plugins.
// A plugin is created and owned by the ExtensionSystem PluginManager.
// It participates in a controlled lifecycle via the virtual functions below.
class AIECAD_EXTENSION_SYSTEM_EXPORT IPlugin : public QObject {
	Q_OBJECT
public:
	enum class ShutdownFlag {
		SynchronousShutdown, AsynchronousShutdown
	};

	explicit IPlugin(QObject* parent = nullptr);
	~IPlugin() override;

	// Called after the plugin has been constructed and its dependencies
	// have been resolved, but before other plugins are fully initialized.
	//
	// arguments:
	//   Command-line or configuration arguments specific to this plugin.
	//
	// errorMessage:
	//   On failure, should be set to a human-readable explanation.
	//
	// Returns:
	//   true  if initialization succeeded
	//   false if initialization failed and the plugin should not be used
	virtual bool initialize(const QStringList &args, QString &errMsg) = 0;

	// Called after ALL enabled plugins have successfully run initialize().
	// At this point, the plugin can safely interact with services published
	// by other plugins via the object pool.
	//
	// Default implementation does nothing.
	virtual void extensionsInitialized();

	// Called once after the main event loop has started. Plugins can use this
	// to perform non-critical, deferred work.
	//
	// Returns:
	//   true  if the plugin has scheduled additional delayed work
	//   false otherwise (default)
	//
	// Default implementation returns false.
	virtual bool delayedInitialization();

	// Called during application shutdown. The plugin can either perform all
	// teardown synchronously and return SynchronousShutdown, or start
	// asynchronous work and return AsynchronousShutdown. In the latter case,
	// the plugin MUST emit asyncShutdownFinished() when done.
	//
	// Default implementation returns SYNC_SHUTDOWN.
	virtual ShutdownFlag aboutToShutdown();

signals:
	// Emitted when an asynchronous shutdown requested by aboutToShutdown()
	// has finished and the plugin can be destroyed safely.
	void asynchronousShutdownFinished();

};
} // namespace aiecad