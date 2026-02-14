#include <gtest/gtest.h>

#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/CanvasCommands.hpp"
#include "canvas/utils/CanvasAutoPorts.hpp"

#include <algorithm>
#include <cmath>

TEST(CanvasPortTests, PortAnchorsAreClampedAwayFromCorners)
{
    Canvas::CanvasBlock blk(QRectF(10.0, 20.0, 100.0, 50.0), false);

    const Canvas::PortId portA = Canvas::PortId::create();
    const Canvas::PortId portB = Canvas::PortId::create();

    std::vector<Canvas::CanvasPort> ports;
    ports.push_back(Canvas::CanvasPort{portA, Canvas::PortRole::Consumer, Canvas::PortSide::Right, 0.0, {}});
    ports.push_back(Canvas::CanvasPort{portB, Canvas::PortRole::Consumer, Canvas::PortSide::Right, 1.0, {}});
    blk.setPorts(std::move(ports));

    const QPointF a0 = blk.portAnchorScene(portA);
    const QPointF a1 = blk.portAnchorScene(portB);

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

TEST(CanvasPortTests, AutoOppositeProducerPortCreatesAndRemoves)
{
    Canvas::CanvasDocument doc;
    auto* blk = doc.createBlock(QRectF(0.0, 0.0, 40.0, 40.0), false);
    ASSERT_NE(blk, nullptr);
    blk->setAutoOppositeProducerPort(true);

    const Canvas::PortId input = blk->addPort(Canvas::PortSide::Left, 0.25, Canvas::PortRole::Consumer);
    ASSERT_TRUE(input);

    EXPECT_TRUE(Canvas::Support::ensureOppositeProducerPort(doc, blk->id(), input));

    Canvas::PortId output{};
    for (const auto& port : blk->ports()) {
        if (port.role == Canvas::PortRole::Producer &&
            port.side == Canvas::PortSide::Right &&
            std::abs(port.t - 0.25) <= 1e-4) {
            output = port.id;
            break;
        }
    }
    ASSERT_TRUE(output);

    EXPECT_FALSE(Canvas::Support::ensureOppositeProducerPort(doc, blk->id(), input));

    auto removed = Canvas::Support::removeOppositeProducerPort(doc, blk->id(), input);
    ASSERT_TRUE(removed.has_value());
    EXPECT_EQ(removed->port.id, output);
}

TEST(CanvasPortTests, AutoOppositeProducerPortRetainsAttached)
{
    Canvas::CanvasDocument doc;
    auto* blk = doc.createBlock(QRectF(0.0, 0.0, 40.0, 40.0), false);
    ASSERT_NE(blk, nullptr);
    blk->setAutoOppositeProducerPort(true);

    const Canvas::PortId input = blk->addPort(Canvas::PortSide::Left, 0.5, Canvas::PortRole::Consumer);
    ASSERT_TRUE(input);

    EXPECT_TRUE(Canvas::Support::ensureOppositeProducerPort(doc, blk->id(), input));

    Canvas::PortId output{};
    for (const auto& port : blk->ports()) {
        if (port.role == Canvas::PortRole::Producer &&
            port.side == Canvas::PortSide::Right &&
            std::abs(port.t - 0.5) <= 1e-4) {
            output = port.id;
            break;
        }
    }
    ASSERT_TRUE(output);

    auto* blkB = doc.createBlock(QRectF(60.0, 0.0, 40.0, 40.0), false);
    ASSERT_NE(blkB, nullptr);
    const Canvas::PortId target = blkB->addPort(Canvas::PortSide::Left, 0.5, Canvas::PortRole::Consumer);
    ASSERT_TRUE(target);

    Canvas::CanvasWire::Endpoint a{Canvas::PortRef{blk->id(), output}, QPointF()};
    Canvas::CanvasWire::Endpoint b{Canvas::PortRef{blkB->id(), target}, QPointF()};
    auto wire = std::make_unique<Canvas::CanvasWire>(a, b);
    wire->setId(doc.allocateId());
    ASSERT_TRUE(doc.commands().execute(std::make_unique<Canvas::CreateItemCommand>(std::move(wire))));

    auto removed = Canvas::Support::removeOppositeProducerPort(doc, blk->id(), input);
    EXPECT_FALSE(removed.has_value());
}
