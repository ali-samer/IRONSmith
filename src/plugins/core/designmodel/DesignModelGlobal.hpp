#pragma once

#include <QtCore/qglobal.h>

#if defined(DESIGNMODEL_LIBRARY)
#  define DESIGNMODEL_EXPORT Q_DECL_EXPORT
#else
#  define DESIGNMODEL_EXPORT Q_DECL_IMPORT
#endif