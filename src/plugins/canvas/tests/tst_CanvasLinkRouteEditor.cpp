#include <gtest/gtest.h>

#include <canvas/CanvasLinkRouteEditor.hpp>

using namespace Canvas;

TEST(CanvasLinkRouteEditor, ShiftsHorizontalRunToNearestLaneWithJogs)
{
    const QVector<double> xs{0.0, 5.0, 10.0};
    const QVector<double> ys{0.0, 5.0, 10.0};
    const QVector<QRectF> obstacles;

    const QVector<QPointF> poly{QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    const auto r = LinkRouteEditor::shiftSegmentToNearestLane(poly,
                                                              0,
                                                              QPointF(2.0, 4.6),
                                                              xs,
                                                              ys,
                                                              obstacles,
                                                              0.0);

    ASSERT_FALSE(r.worldPoints.isEmpty());
    EXPECT_TRUE(r.ok);
    EXPECT_TRUE(r.horizontalRun);
    EXPECT_NEAR(r.snappedCoord, 5.0, 1e-6);

    const QVector<QPointF> expected{
        QPointF(0.0, 0.0),
        QPointF(0.0, 5.0),
        QPointF(10.0, 5.0),
        QPointF(10.0, 0.0),
        QPointF(10.0, 10.0)
    };
    EXPECT_EQ(r.worldPoints, expected);
}

TEST(CanvasLinkRouteEditor, RejectsShiftThatIntersectsObstacle)
{
    const QVector<double> xs{0.0, 5.0, 10.0};
    const QVector<double> ys{0.0, 5.0, 10.0};

    const QVector<QRectF> obstacles{QRectF(2.0, 4.0, 6.0, 2.0)};

    const QVector<QPointF> poly{QPointF(0.0, 0.0), QPointF(10.0, 0.0), QPointF(10.0, 10.0)};

    const auto r = LinkRouteEditor::shiftSegmentToNearestLane(poly,
                                                              0,
                                                              QPointF(2.0, 5.0),
                                                              xs,
                                                              ys,
                                                              obstacles,
                                                              0.0);
    EXPECT_FALSE(r.ok);
    EXPECT_GE(r.worldPoints.size(), 3);
}