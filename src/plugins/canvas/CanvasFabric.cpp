#include "canvas/CanvasFabric.hpp"

#include <QtGui/QPainter>

#include <QtCore/QtMath>

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

void CanvasFabric::draw(QPainter& p,
                        const QRectF& sceneRect,
                        IsBlockedFn isBlocked,
                        void* user) const
{
    const auto coords = enumerate(sceneRect, isBlocked, user);
    if (coords.empty())
        return;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(m_cfg.pointColor));

    const bool innerEnabled = m_cfg.pointInnerRadius > 0.0;
    const QColor innerColor(m_cfg.pointInnerColor);

    const double step = m_cfg.step;

    for (const auto& c : coords) {
        const QPointF scenePos(c.x * step, c.y * step);
        p.drawEllipse(scenePos, m_cfg.pointRadius, m_cfg.pointRadius);

        if (innerEnabled) {
            p.setBrush(innerColor);
            p.drawEllipse(scenePos, m_cfg.pointInnerRadius, m_cfg.pointInnerRadius);
            p.setBrush(QColor(m_cfg.pointColor));
        }
    }
}

std::vector<FabricCoord> CanvasFabric::enumerate(const QRectF& sceneRect,
                                                 IsBlockedFn isBlocked,
                                                 void* user) const
{
    const QRectF r = normalizedRect(sceneRect);
    if (r.isEmpty() || qFuzzyIsNull(m_cfg.step))
        return {};

    const double step = m_cfg.step;
    const double pad = step;
    const double minX = r.left()   - pad;
    const double maxX = r.right()  + pad;
    const double minY = r.top()    - pad;
    const double maxY = r.bottom() + pad;

    const int ix0 = static_cast<int>(qFloor(minX / step));
    const int ix1 = static_cast<int>(qCeil (maxX / step));
    const int iy0 = static_cast<int>(qFloor(minY / step));
    const int iy1 = static_cast<int>(qCeil (maxY / step));

    std::vector<FabricCoord> out;
    out.reserve(static_cast<size_t>((ix1 - ix0 + 1) * (iy1 - iy0 + 1)));

    for (int y = iy0; y <= iy1; ++y) {
        for (int x = ix0; x <= ix1; ++x) {
            const FabricCoord c{x, y};
            if (isBlocked && isBlocked(c, user))
                continue;
            out.push_back(c);
        }
    }

    return out;
}

} // namespace Canvas