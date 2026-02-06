#pragma once

#include <QtCore/qglobal.h>

#if defined(COMMANDPLUGIN_LIBRARY)
#  define COMMANDPLUGIN_EXPORT Q_DECL_EXPORT
#else
#  define COMMANDPLUGIN_EXPORT Q_DECL_IMPORT
#endif
