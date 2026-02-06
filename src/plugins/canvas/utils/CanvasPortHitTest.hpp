#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>

namespace Canvas::Utils {

bool hitTestPortGeometry(const QPointF& anchorScene,
                         PortSide side,
                         const QPointF& scenePos,
                         double radiusScene);

} // namespace Canvas::Utils
