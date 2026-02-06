#include "utils/ui/GridLayout.hpp"

#include <QtCore/QMarginsF>
#include <QtCore/QSizeF>
#include <QtCore/QtMath>

namespace Utils {

QSizeF GridLayout::resolveCellSize(const GridSpec& spec,
                                  const QSizeF& viewportSize,
                                  double fallbackCellSize)
{
    if (spec.hasExplicitCellSize())
        return spec.cellSize;

    const double fallback = fallbackCellSize > 0.0 ? fallbackCellSize : 1.0;

    if (spec.columns <= 0 || spec.rows <= 0)
        return QSizeF(fallback, fallback);

    if (viewportSize.width() <= 0.0 || viewportSize.height() <= 0.0)
        return QSizeF(fallback, fallback);

    const double spacingW = qMax(0.0, spec.cellSpacing.width());
    const double spacingH = qMax(0.0, spec.cellSpacing.height());

    const double availW = viewportSize.width()
                          - spec.outerMargin.left()
                          - spec.outerMargin.right()
                          - (spec.columns - 1) * spacingW;
    const double availH = viewportSize.height()
                          - spec.outerMargin.top()
                          - spec.outerMargin.bottom()
                          - (spec.rows - 1) * spacingH;

    if (availW <= 0.0 || availH <= 0.0)
        return QSizeF(fallback, fallback);

    const double cellW = availW / spec.columns;
    const double cellH = availH / spec.rows;
    const double cell = qMin(cellW, cellH);

    if (cell <= 0.0)
        return QSizeF(fallback, fallback);

    return QSizeF(cell, cell);
}

QRectF GridLayout::rectForGrid(const GridSpec& spec,
                               const GridRect& rect,
                               const QSizeF& cellSize)
{
    const double spacingW = qMax(0.0, spec.cellSpacing.width());
    const double spacingH = qMax(0.0, spec.cellSpacing.height());

    const double width = rect.columnSpan * cellSize.width()
                         + qMax(0, rect.columnSpan - 1) * spacingW;
    const double height = rect.rowSpan * cellSize.height()
                          + qMax(0, rect.rowSpan - 1) * spacingH;

    const double x = spec.outerMargin.left()
                     + rect.column * (cellSize.width() + spacingW);

    double y = 0.0;
    if (spec.origin == GridOrigin::TopLeft) {
        const int fromTop = spec.rows - rect.row - rect.rowSpan;
        y = spec.outerMargin.top() + fromTop * (cellSize.height() + spacingH);
    } else {
        y = spec.outerMargin.bottom() + rect.row * (cellSize.height() + spacingH);
    }

    return QRectF(QPointF(x, y), QSizeF(width, height));
}

} // namespace Utils
