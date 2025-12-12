// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#include "IPlugin.hpp"

namespace aiecad {
IPlugin::IPlugin(QObject *parent) : QObject(parent) {}

IPlugin::~IPlugin() = default;

void IPlugin::extensionsInitialized() {
}

bool IPlugin::delayedInitialization() {
	return false;
}

IPlugin::ShutdownFlag IPlugin::aboutToShutdown() {
	return ShutdownFlag::SynchronousShutdown;
}

} // namespace aiecad