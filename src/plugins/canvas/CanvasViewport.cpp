#include "canvas/CanvasViewport.hpp"

#include "canvas/Tools.hpp"

#include <QtCore/QtGlobal>
#include <algorithm>

namespace Canvas {

CanvasViewport::CanvasViewport(QObject* parent)
    : QObject(parent)
{
    m_zoom = Tools::clampZoom(1.0);
}

double CanvasViewport::displayZoom() const noexcept
{
    if (qFuzzyIsNull(m_displayZoomBaseline))
        return m_zoom;
    return m_zoom / m_displayZoomBaseline;
}

void CanvasViewport::setZoom(double zoom)
{
    const double clamped = Tools::clampZoom(zoom);
    if (qFuzzyCompare(m_zoom, clamped))
        return;
    m_zoom = clamped;
    emit zoomChanged(m_zoom);
}

void CanvasViewport::setDisplayZoomBaseline(double baseline)
{
    if (baseline <= 0.0)
        return;
    if (qFuzzyCompare(m_displayZoomBaseline, baseline))
        return;
    m_displayZoomBaseline = baseline;
    emit displayZoomBaselineChanged(m_displayZoomBaseline);
}

void CanvasViewport::setPan(const QPointF& pan)
{
    if (m_pan == pan)
        return;
    const QPointF deltaScene = pan - m_pan;
    m_pan = pan;
    if (!qFuzzyIsNull(m_zoom)) {
        const QPointF deltaView(deltaScene.x() * m_zoom, deltaScene.y() * m_zoom);
        if (!qFuzzyIsNull(deltaView.x()) || !qFuzzyIsNull(deltaView.y()))
            emit panDeltaView(deltaView);
    }
    emit panChanged(m_pan);
}

void CanvasViewport::setSize(const QSizeF& size)
{
    if (m_size == size)
        return;
    m_size = size;
    emit sizeChanged(m_size);
}

QPointF CanvasViewport::viewToScene(const QPointF& viewPos) const
{
    return Tools::viewToScene(viewPos, m_pan, m_zoom);
}

QPointF CanvasViewport::sceneToView(const QPointF& scenePos) const
{
    return Tools::sceneToView(scenePos, m_pan, m_zoom);
}

QRectF CanvasViewport::visibleSceneRect() const
{
    if (m_size.isEmpty())
        return QRectF();

    const QPointF tl = viewToScene(QPointF(0.0, 0.0));
    const QPointF br = viewToScene(QPointF(m_size.width(), m_size.height()));
    const double left = std::min(tl.x(), br.x());
    const double right = std::max(tl.x(), br.x());
    const double top = std::min(tl.y(), br.y());
    const double bottom = std::max(tl.y(), br.y());
    return QRectF(QPointF(left, top), QPointF(right, bottom));
}

} // namespace Canvas
