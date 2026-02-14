#include "canvas/CanvasWire.hpp"
#include "canvas/CanvasConstants.hpp"
#include "canvas/utils/CanvasLinkWireStyle.hpp"

#include <gtest/gtest.h>

namespace Canvas::Tests {

TEST(CanvasWireStyleTests, ColorOverrideAndArrowPolicyClone)
{
    CanvasWire::Endpoint a;
    CanvasWire::Endpoint b;
    CanvasWire wire(a, b);

    const QColor overrideColor("#123456");
    wire.setColorOverride(overrideColor);
    wire.setArrowPolicy(WireArrowPolicy::Start);

    auto clone = wire.clone();
    auto* clonedWire = dynamic_cast<CanvasWire*>(clone.get());
    ASSERT_TRUE(clonedWire != nullptr);
    EXPECT_TRUE(clonedWire->hasColorOverride());
    EXPECT_EQ(clonedWire->colorOverride().name(QColor::HexArgb),
              overrideColor.name(QColor::HexArgb));
    EXPECT_EQ(clonedWire->arrowPolicy(), WireArrowPolicy::Start);
}

TEST(CanvasLinkWireStyleTests, RoleColorsMatchConstants)
{
    const auto producer = Support::linkWireStyle(Support::LinkWireRole::Producer);
    const auto consumer = Support::linkWireStyle(Support::LinkWireRole::Consumer);

    EXPECT_EQ(producer.color.name(QColor::HexArgb),
              QColor(Constants::kLinkWireProducerColor).name(QColor::HexArgb));
    EXPECT_EQ(consumer.color.name(QColor::HexArgb),
              QColor(Constants::kLinkWireConsumerColor).name(QColor::HexArgb));
}

} // namespace Canvas::Tests
