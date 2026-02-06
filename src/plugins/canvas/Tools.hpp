#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasTypes.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <functional>
#include <optional>

namespace Canvas {
class CanvasDocument;

namespace Tools {

// -----------------------------------------------------------------------------
// Math utilities
// -----------------------------------------------------------------------------
namespace Math {

CANVAS_EXPORT double clampZoom(double z);

CANVAS_EXPORT QPointF sceneToView(const QPointF& scenePos, const QPointF& pan, double zoom);
CANVAS_EXPORT QPointF viewToScene(const QPointF& viewPos, const QPointF& pan, double zoom);

CANVAS_EXPORT QPointF panFromViewDrag(const QPointF& startPan,
                                      const QPointF& startViewPos,
                                      const QPointF& currentViewPos,
                                      double zoom);

} // namespace Math

struct CANVAS_EXPORT Toolbox final
{
    // Services::HitTestTool* hitTest = nullptr;
};

// -----------------------------------------------------------------------------
// Legacy/compat wrappers.
// -----------------------------------------------------------------------------
CANVAS_EXPORT double clampZoom(double z);
CANVAS_EXPORT QPointF sceneToView(const QPointF& scenePos, const QPointF& pan, double zoom);
CANVAS_EXPORT QPointF viewToScene(const QPointF& viewPos, const QPointF& pan, double zoom);
CANVAS_EXPORT QPointF panFromViewDrag(const QPointF& startPan,
                                      const QPointF& startViewPos,
                                      const QPointF& currentViewPos,
                                      double zoom);

} // namespace Tools
} // namespace Canvas
