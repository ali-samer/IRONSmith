// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasRenderContext.hpp"

#include <QtCore/QPointF>

namespace Canvas {
class CanvasBlock;
class CanvasDocument;
class CanvasItem;
}

namespace Canvas::Services {

CanvasItem* hitTestItem(const CanvasDocument& doc,
                        const QPointF& scenePos,
                        const CanvasRenderContext* ctx = nullptr);

CanvasBlock* hitTestBlock(const CanvasDocument& doc,
                          const QPointF& scenePos);

} // namespace Canvas::Services
