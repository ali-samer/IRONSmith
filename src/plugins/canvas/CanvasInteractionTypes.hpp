// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>

namespace Canvas {

struct EdgeCandidate final {
    ObjectId itemId{};
    PortSide side = PortSide::Left;
    double t = 0.5;
    QPointF anchorScene;
};

} // namespace Canvas
