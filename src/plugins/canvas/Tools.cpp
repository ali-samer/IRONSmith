#include "canvas/Tools.hpp"
#include "canvas/CanvasConstants.hpp"

#include <algorithm>
#include <QtCore/qmath.h>

#include "canvas/CanvasDocument.hpp"

namespace Canvas::Tools::Math {

double clampZoom(double z)
{
    return std::clamp(z, Constants::kMinZoom, Constants::kMaxZoom);
}

QPointF sceneToView(const QPointF& scenePos, const QPointF& pan, double zoom)
{
    return (scenePos + pan) * zoom;
}

QPointF viewToScene(const QPointF& viewPos, const QPointF& pan, double zoom)
{
    return (viewPos / zoom) - pan;
}

QPointF panFromViewDrag(const QPointF& startPan,
                        const QPointF& startViewPos,
                        const QPointF& currentViewPos,
                        double zoom)
{
    if (qFuzzyIsNull(zoom))
        return startPan;

    const QPointF deltaView = currentViewPos - startViewPos;
    return startPan + (deltaView / zoom);
}

} // namespace Canvas::Tools::Math

namespace Canvas::Tools {

double clampZoom(double z) { return Math::clampZoom(z); }

QPointF sceneToView(const QPointF& scenePos, const QPointF& pan, double zoom)
{
    return Math::sceneToView(scenePos, pan, zoom);
}

QPointF viewToScene(const QPointF& viewPos, const QPointF& pan, double zoom)
{
    return Math::viewToScene(viewPos, pan, zoom);
}

QPointF panFromViewDrag(const QPointF& startPan,
                        const QPointF& startViewPos,
                        const QPointF& currentViewPos,
                        double zoom)
{
    return Math::panFromViewDrag(startPan, startViewPos, currentViewPos, zoom);
}

} // namespace Canvas::Tools
