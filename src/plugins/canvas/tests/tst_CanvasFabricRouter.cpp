#include <gtest/gtest.h>

#include <canvas/CanvasFabricRouter.hpp>

#include <algorithm>
#include <cmath>

using namespace Canvas;

namespace {

bool isOnAxis(const QVector<double>& axis, double v)
{
    for (double x : axis) {
        if (std::abs(x - v) < 1e-6)
            return true;
    }
    return false;
}

bool segmentIntersectsInterior(const QPointF& a, const QPointF& b, const QRectF& r)
{
    const double eps = 0.25;
    const QRectF interior(r.left() + eps,
                         r.top() + eps,
                         r.width() - 2.0 * eps,
                         r.height() - 2.0 * eps);
    if (interior.isEmpty())
        return false;

    if (std::abs(a.y() - b.y()) < 1e-6) {
        const double y = a.y();
        if (!(y > interior.top() && y < interior.bottom()))
            return false;
        const double x1 = std::min(a.x(), b.x());
        const double x2 = std::max(a.x(), b.x());
        return (x2 > interior.left() && x1 < interior.right());
    }

    if (std::abs(a.x() - b.x()) < 1e-6) {
        const double x = a.x();
        if (!(x > interior.left() && x < interior.right()))
            return false;
        const double y1 = std::min(a.y(), b.y());
        const double y2 = std::max(a.y(), b.y());
        return (y2 > interior.top() && y1 < interior.bottom());
    }

    return true;
}

} // namespace

TEST(CanvasFabricRouter, DeterministicOrthogonalPathOnLattice)
{
    const QVector<double> xs{0.0, 10.0, 20.0, 30.0};
    const QVector<double> ys{0.0, 10.0, 20.0};

    const QRectF obstacle(9.0, 9.0, 2.0, 2.0);
    const QVector<QRectF> obstacles{obstacle};

    const QPointF start(0.0, 0.0);
    const QPointF end(30.0, 20.0);

    const QVector<QPointF> p1 = FabricRouter::route(start, end, xs, ys, obstacles);
    const QVector<QPointF> p2 = FabricRouter::route(start, end, xs, ys, obstacles);

    ASSERT_FALSE(p1.isEmpty());
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1.front(), start);
    EXPECT_EQ(p1.back(), end);

    for (int i = 0; i < p1.size(); ++i) {
        EXPECT_TRUE(isOnAxis(xs, p1[i].x()));
        EXPECT_TRUE(isOnAxis(ys, p1[i].y()));
    }

    for (int i = 0; i+1 < p1.size(); ++i) {
        const QPointF a = p1[i];
        const QPointF b = p1[i + 1];
        EXPECT_TRUE(std::abs(a.x() - b.x()) < 1e-6 || std::abs(a.y() - b.y()) < 1e-6);
        EXPECT_FALSE(segmentIntersectsInterior(a, b, obstacle));
    }
}

TEST(CanvasFabricRouter, FallsBackToStraightSegmentWhenOffLattice)
{
    const QVector<double> xs{0.0, 10.0};
    const QVector<double> ys{0.0, 10.0};
    const QVector<QRectF> obstacles;

    const QPointF start(1.0, 0.0); // off lattice
    const QPointF end(10.0, 10.0);

    const QVector<QPointF> p = FabricRouter::route(start, end, xs, ys, obstacles);
    ASSERT_EQ(p.size(), 2);
    EXPECT_EQ(p[0], start);
    EXPECT_EQ(p[1], end);
}