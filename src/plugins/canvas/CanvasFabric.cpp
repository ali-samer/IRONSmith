#include "canvas/CanvasFabric.hpp"

#include <QtGui/QPainter>

#include <QtCore/QtMath>
#include <QtGui/QTransform>
#include <QtCore/QVector>

namespace Canvas {

static QRectF normalizedRect(const QRectF& r)
{
    QRectF out = r;
    if (out.width() < 0) {
        out.setLeft(r.right());
        out.setRight(r.left());
    }
    if (out.height() < 0) {
        out.setTop(r.bottom());
        out.setBottom(r.top());
    }
    return out;
}

static double painterScale(const QPainter& p)
{
    const QTransform t = p.worldTransform();
    const double sx = std::hypot(t.m11(), t.m21());
    const double sy = std::hypot(t.m22(), t.m12());
    return std::max(sx, sy);
}

static int strideForPainter(const QPainter& p, double step)
{
    if (step <= 0.0)
        return 1;

    const double scale = painterScale(p);
    if (qFuzzyIsNull(scale))
        return 1;

    const double deviceStep = step * scale;
    if (deviceStep <= 0.0)
        return 1;

    constexpr double kMinPixelSpacing = 6.0;
    const int stride = static_cast<int>(std::ceil(kMinPixelSpacing / deviceStep));
    return std::max(1, stride);
}

static std::vector<FabricCoord> enumerateCoords(const QRectF& sceneRect,
                                                double step,
                                                int stride,
                                                CanvasFabric::IsBlockedFn isBlocked,
                                                void* user)
{
    const QRectF r = normalizedRect(sceneRect);
    if (r.isEmpty() || qFuzzyIsNull(step))
        return {};

    stride = std::max(1, stride);
    const double pad = step;
    const double minX = r.left()   - pad;
    const double maxX = r.right()  + pad;
    const double minY = r.top()    - pad;
    const double maxY = r.bottom() + pad;

    const int ix0 = static_cast<int>(qFloor(minX / step));
    const int ix1 = static_cast<int>(qCeil (maxX / step));
    const int iy0 = static_cast<int>(qFloor(minY / step));
    const int iy1 = static_cast<int>(qCeil (maxY / step));

    const int spanX = std::max(0, ix1 - ix0);
    const int spanY = std::max(0, iy1 - iy0);
    const int countX = spanX / stride + 1;
    const int countY = spanY / stride + 1;

    std::vector<FabricCoord> out;
    out.reserve(static_cast<size_t>(countX) * static_cast<size_t>(countY));

    for (int y = iy0; y <= iy1; y += stride) {
        for (int x = ix0; x <= ix1; x += stride) {
            const FabricCoord c{x, y};
            if (isBlocked && isBlocked(c, user))
                continue;
            out.push_back(c);
        }
    }

    return out;
}

void CanvasFabric::draw(QPainter& p,
                        const QRectF& sceneRect,
                        IsBlockedFn isBlocked,
                        void* user) const
{
    const int stride = strideForPainter(p, m_cfg.step);
    const auto coords = enumerateCoords(sceneRect, m_cfg.step, stride, isBlocked, user);
    if (coords.empty())
        return;

    const double step = m_cfg.step;
    QVector<QPointF> points;
    points.reserve(static_cast<int>(coords.size()));
    for (const auto& c : coords)
        points.push_back(QPointF(c.x * step, c.y * step));

    p.setBrush(Qt::NoBrush);
    QPen outerPen(QColor(m_cfg.pointColor));
    outerPen.setWidthF(std::max(0.0, m_cfg.pointRadius * 2.0));
    outerPen.setCapStyle(Qt::RoundCap);
    outerPen.setJoinStyle(Qt::RoundJoin);
    p.setPen(outerPen);
    p.drawPoints(points.constData(), points.size());

    const bool innerEnabled = m_cfg.pointInnerRadius > 0.0;
    if (!innerEnabled)
        return;

    QPen innerPen(QColor(m_cfg.pointInnerColor));
    innerPen.setWidthF(std::max(0.0, m_cfg.pointInnerRadius * 2.0));
    innerPen.setCapStyle(Qt::RoundCap);
    innerPen.setJoinStyle(Qt::RoundJoin);
    p.setPen(innerPen);
    p.drawPoints(points.constData(), points.size());
}

std::vector<FabricCoord> CanvasFabric::enumerate(const QRectF& sceneRect,
                                                 IsBlockedFn isBlocked,
                                                 void* user) const
{
    return enumerateCoords(sceneRect, m_cfg.step, 1, isBlocked, user);
}

} // namespace Canvas
