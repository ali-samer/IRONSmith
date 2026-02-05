#pragma once

#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

#include <algorithm>
#include <cmath>

namespace Canvas::Utils {

inline double snapCoord(double v, double step)
{
    if (step <= 0.0)
        return v;
    return std::llround(v / step) * step;
}

inline QPointF snapPointToGrid(const QPointF& p, double step)
{
    return QPointF(snapCoord(p.x(), step), snapCoord(p.y(), step));
}

inline double snapSizeUp(double v, double step)
{
    if (step <= 0.0)
        return v;
    return std::ceil(v / step) * step;
}

inline QRectF snapBoundsToGrid(const QRectF& r, double step)
{
    if (step <= 0.0)
        return r;

    QRectF out = r;
    const QPointF tl = r.topLeft();
    const double w = r.width();
    const double h = r.height();

    const QPointF snappedTopLeft(snapCoord(tl.x(), step), snapCoord(tl.y(), step));
    const double snappedW = snapSizeUp(w, step);
    const double snappedH = snapSizeUp(h, step);
    out.setTopLeft(snappedTopLeft);
    out.setSize(QSizeF(snappedW, snappedH));
    return out;
}

inline double clampT(double t, double lo = 0.10, double hi = 0.90)
{
    return t < lo ? lo : (t > hi ? hi : t);
}

inline FabricCoord toFabricCoord(const QPointF& s, double step)
{
    const int ix = static_cast<int>(std::llround(s.x() / step));
    const int iy = static_cast<int>(std::llround(s.y() / step));
    return FabricCoord{ix, iy};
}

inline QPointF toScenePoint(const FabricCoord& c, double step)
{
    return QPointF(c.x * step, c.y * step);
}

} // namespace Canvas::Utils
