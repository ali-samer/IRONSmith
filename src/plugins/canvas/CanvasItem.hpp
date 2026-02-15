// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"
#include "canvas/CanvasRenderContext.hpp"
#include "canvas/CanvasTypes.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <qnamespace.h>

#include <memory>
#include <vector>

QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

namespace Canvas {

class CANVAS_EXPORT CanvasItem
{
public:
    virtual ~CanvasItem() = default;

    ObjectId id() const noexcept { return m_id; }

    virtual void draw(QPainter& p, const CanvasRenderContext& ctx) const = 0;
    virtual QRectF boundsScene() const = 0;

    virtual std::unique_ptr<CanvasItem> clone() const = 0;

    virtual bool hitTest(const QPointF& scenePos) const {
        return boundsScene().contains(scenePos);
    }

    virtual bool blocksFabric() const { return false; }
    virtual QRectF keepoutSceneRect() const { return QRectF(); }

    virtual bool hasPorts() const { return false; }
    virtual const std::vector<CanvasPort>& ports() const {
        static const std::vector<CanvasPort> kEmpty{};
        return kEmpty;
    }
    virtual QPointF portAnchorScene(PortId) const { return QPointF(); }

    void setId(ObjectId id) noexcept { m_id = id; }

private:
    ObjectId m_id{};
};

} // namespace Canvas
