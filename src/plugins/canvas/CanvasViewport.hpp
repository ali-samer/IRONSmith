#pragma once

#include <QtCore/QPointF>
#include <QtCore/QVector>

namespace Canvas {

class CanvasViewport final
{
public:
    CanvasViewport()
        : m_zoomFactors({0.5, 0.75, 1.0, 1.25, 1.5, 2.0, 3.0, 4.0})
        , m_zoomIndex(2)
        , m_pan(0.0, 0.0)
    {
    }

    int zoomIndex() const noexcept { return m_zoomIndex; }
    double zoomFactor() const noexcept { return m_zoomFactors[m_zoomIndex]; }

    void setZoomIndex(int idx)
    {
        if (m_zoomFactors.isEmpty()) {
            m_zoomIndex = 0;
            return;
        }
        if (idx < 0)
            idx = 0;
        if (idx >= m_zoomFactors.size())
            idx = m_zoomFactors.size() - 1;
        m_zoomIndex = idx;
    }

    void stepZoom(int deltaSteps)
    {
        setZoomIndex(m_zoomIndex + deltaSteps);
    }

    QPointF pan() const noexcept { return m_pan; }
    void setPan(QPointF p) { m_pan = p; }
    void panBy(QPointF delta) { m_pan += delta; }

    QPointF worldToScreen(QPointF world) const noexcept
    {
        const double z = zoomFactor();
        return QPointF(world.x() * z + m_pan.x(), world.y() * z + m_pan.y());
    }

    QPointF screenToWorld(QPointF screen) const noexcept
    {
        const double z = zoomFactor();
        return QPointF((screen.x() - m_pan.x()) / z, (screen.y() - m_pan.y()) / z);
    }

private:
    QVector<double> m_zoomFactors;
    int m_zoomIndex{0};
    QPointF m_pan{0.0, 0.0};
};

} // namespace Canvas
