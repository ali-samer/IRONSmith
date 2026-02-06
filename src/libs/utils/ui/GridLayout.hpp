#pragma once

#include "utils/UtilsGlobal.hpp"
#include "utils/ui/GridSpec.hpp"

#include <QtCore/QRectF>
#include <QtCore/QSizeF>

namespace Utils {

class UTILS_EXPORT GridLayout final
{
public:
    static QSizeF resolveCellSize(const GridSpec& spec,
                                 const QSizeF& viewportSize,
                                 double fallbackCellSize);

    static QRectF rectForGrid(const GridSpec& spec,
                              const GridRect& rect,
                              const QSizeF& cellSize);
};

} // namespace Utils
