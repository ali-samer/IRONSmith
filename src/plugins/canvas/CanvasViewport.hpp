// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "canvas/CanvasGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

namespace Canvas {

class CANVAS_EXPORT CanvasViewport final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(QSizeF size READ size WRITE setSize NOTIFY sizeChanged)

public:
    explicit CanvasViewport(QObject* parent = nullptr);

    double zoom() const noexcept { return m_zoom; }
    double displayZoom() const noexcept;
    double displayZoomBaseline() const noexcept { return m_displayZoomBaseline; }
    QPointF pan() const noexcept { return m_pan; }
    QSizeF size() const noexcept { return m_size; }

    void setZoom(double zoom);
    void setDisplayZoomBaseline(double baseline);
    void setPan(const QPointF& pan);
    void setSize(const QSizeF& size);

    QPointF viewToScene(const QPointF& viewPos) const;
    QPointF sceneToView(const QPointF& scenePos) const;
    QRectF visibleSceneRect() const;

signals:
    void zoomChanged(double zoom);
    void displayZoomBaselineChanged(double baseline);
    void panChanged(QPointF pan);
    void panDeltaView(QPointF deltaView);
    void sizeChanged(QSizeF size);

private:
    double m_zoom = 1.0;
    double m_displayZoomBaseline = 1.0;
    QPointF m_pan = {0.0, 0.0};
    QSizeF m_size;
};

} // namespace Canvas
