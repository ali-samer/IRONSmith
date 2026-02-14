#pragma once

#include "utils/UtilsGlobal.hpp"

#include <QtCore/QDir>
#include <QtCore/QString>

namespace Utils::FileSystemUtils {

UTILS_EXPORT QString uniqueChildName(const QDir& dir, const QString& baseName, const QString& ext);
UTILS_EXPORT QString duplicateName(const QDir& dir, const QString& fileName);

} // namespace Utils::FileSystemUtils
