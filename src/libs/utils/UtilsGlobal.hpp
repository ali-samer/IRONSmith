// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QtGlobal>

#if defined(UTILS_BUILD_SHARED) && (UTILS_BUILD_SHARED == 1)
#	if defined(UTILS_LIBRARY)
#		define UTILS_EXPORT Q_DECL_EXPORT
#	else
#		define UTILS_EXPORT Q_DECL_IMPORT
#	endif
#else
#	define UTILS_EXPORT
#endif
