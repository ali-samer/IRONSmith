// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>

namespace Canvas::Support {

bool hitTestPortGeometry(const QPointF& anchorScene,
                         PortSide side,
                         const QPointF& scenePos,
                         double radiusScene);

} // namespace Canvas::Support
