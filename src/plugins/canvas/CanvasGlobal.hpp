// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QtGlobal>
#include <QtCore/QLoggingCategory>

#if defined(CANVAS_BUILD_SHARED) && (CANVAS_BUILD_SHARED == 1)
#	if defined(CANVAS_LIBRARY)
#		define CANVAS_EXPORT Q_DECL_EXPORT
#	else
#		define CANVAS_EXPORT Q_DECL_IMPORT
#	endif
#else
#	define CANVAS_EXPORT
#endif

Q_DECLARE_LOGGING_CATEGORY(canvaslog)
