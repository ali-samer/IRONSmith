// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Modifications Copyright (C) 2025 Samer Ali
// This file contains modifications for a university capstone project.

#pragma once

#include <QtGlobal>
#include <QtCore/QLoggingCategory>

#if defined(AIECAD_EXTENSION_SYSTEM_SHARED_LIBRARY)
#	define AIECAD_EXTENSION_SYSTEM_EXPORT Q_DECL_EXPORT
#elif defined(AIECAD_EXTENSION_SYSTEM_STATIC_LIBRARY)
#	define AIECAD_EXTENSION_SYSTEM_EXPORT
#else
#	define AIECAD_EXTENSION_SYSTEM_EXPORT Q_DECL_IMPORT
#endif

Q_DECLARE_LOGGING_CATEGORY(aiecadExtensionSystemLog)