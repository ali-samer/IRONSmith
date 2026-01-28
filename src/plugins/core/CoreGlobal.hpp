#pragma once

#include <QtCore/QtGlobal>
#include <QtCore/QLoggingCategory>

#if defined(CORE_BUILD_SHARED) && (CORE_BUILD_SHARED == 1)
#	if defined(CORE_LIBRARY)
#		define CORE_EXPORT Q_DECL_EXPORT
#	else
#		define CORE_EXPORT Q_DECL_IMPORT
#	endif
#else
#	define CORE_EXPORT
#endif

Q_DECLARE_LOGGING_CATEGORY(corelog)