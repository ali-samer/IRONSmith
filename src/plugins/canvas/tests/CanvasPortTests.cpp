#include <gtest/gtest.h>

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasConstants.hpp"

#include <algorithm>
#include <cmath>

TEST(CanvasPortTests, PortAnchorsAreClampedAwayFromCorners)
{
    Canvas::CanvasBlock blk(QRectF(10.0, 20.0, 100.0, 50.0), false);

    std::vector<Canvas::CanvasPort> ports;

    ports.push_back(Canvas::CanvasPort{Canvas::PortId(1), Canvas::PortRole::Consumer, Canvas::PortSide::Right, 0.0, {}});
    ports.push_back(Canvas::CanvasPort{Canvas::PortId(2), Canvas::PortRole::Consumer, Canvas::PortSide::Right, 1.0, {}});
    blk.setPorts(std::move(ports));

    const QPointF a0 = blk.portAnchorScene(Canvas::PortId(1));
    const QPointF a1 = blk.portAnchorScene(Canvas::PortId(2));

    EXPECT_DOUBLE_EQ(a0.x(), blk.boundsScene().right());
    EXPECT_DOUBLE_EQ(a1.x(), blk.boundsScene().right());

    const double step = Canvas::Constants::kGridStep;
    const double raw0 = blk.boundsScene().top() + 0.10 * blk.boundsScene().height();
    const double raw1 = blk.boundsScene().top() + 0.90 * blk.boundsScene().height();
    const double minY = blk.boundsScene().top() + step;
    const double maxY = blk.boundsScene().bottom() - step;

    auto snap = [&](double v) -> double {
        if (step <= 0.0)
            return v;
        return std::llround(v / step) * step;
    };

    const double y0 = std::clamp(snap(raw0), minY, maxY);
    const double y1 = std::clamp(snap(raw1), minY, maxY);

    EXPECT_DOUBLE_EQ(a0.y(), y0);
    EXPECT_DOUBLE_EQ(a1.y(), y1);
}