// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QtGlobal>

#if defined(PROJECTEXPLORER_BUILD_SHARED) && (PROJECTEXPLORER_BUILD_SHARED == 1)
#	if defined(PROJECTEXPLORER_LIBRARY)
#		define PROJECTEXPLORER_EXPORT Q_DECL_EXPORT
#	else
#		define PROJECTEXPLORER_EXPORT Q_DECL_IMPORT
#	endif
#else
#	define PROJECTEXPLORER_EXPORT
#endif

Q_DECLARE_LOGGING_CATEGORY(projectexplorerlog)
