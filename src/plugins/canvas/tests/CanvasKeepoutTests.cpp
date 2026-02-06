#include <gtest/gtest.h>

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasBlock.hpp"

#include <QtCore/QSizeF>

namespace {

bool containsCoord(const std::vector<Canvas::FabricCoord>& v, int x, int y)
{
    for (const auto& c : v) {
        if (c.x == x && c.y == y)
            return true;
    }
    return false;
}

} // namespace

TEST(CanvasKeepoutTests, BlockCarvesHoleInFabricEnumeration)
{
    Canvas::CanvasDocument doc;

    Canvas::CanvasFabric::Config cfg;
    cfg.step = 10.0;
    doc.fabric().setConfig(cfg);

    auto* blk = doc.createBlock(QRectF(QPointF(0.0, 0.0), QSizeF(20.0, 20.0)), false);
    ASSERT_NE(blk, nullptr);
    blk->setKeepoutMargin(0.0);

    const QRectF visible(QPointF(0.0, 0.0), QPointF(20.0, 20.0));
    const auto coords = doc.fabric().enumerate(visible, &Canvas::CanvasDocument::isFabricPointBlockedThunk, &doc);

    EXPECT_FALSE(containsCoord(coords, 0, 0));
    EXPECT_FALSE(containsCoord(coords, 1, 1));
    EXPECT_FALSE(containsCoord(coords, 2, 2));

    EXPECT_TRUE(containsCoord(coords, -1, -1));
    EXPECT_TRUE(containsCoord(coords, 3, 3));
    EXPECT_TRUE(containsCoord(coords, 3, 0));
}

TEST(CanvasKeepoutTests, FixedVsMovableAffectsDraggingButNotKeepoutQuery)
{
    Canvas::CanvasDocument doc;

    Canvas::CanvasFabric::Config cfg;
    cfg.step = 10.0;
    doc.fabric().setConfig(cfg);

    auto* fixed = doc.createBlock(QRectF(QPointF(0.0, 0.0), QSizeF(20.0, 20.0)), false);
    auto* movable = doc.createBlock(QRectF(QPointF(40.0, 0.0), QSizeF(20.0, 20.0)), true);
    ASSERT_NE(fixed, nullptr);
    ASSERT_NE(movable, nullptr);
    fixed->setKeepoutMargin(0.0);
    movable->setKeepoutMargin(0.0);

    ASSERT_TRUE(doc.setItemTopLeft(movable->id(), QPointF(50.0, 0.0)));

    EXPECT_TRUE(doc.isFabricPointBlocked(Canvas::FabricCoord{0, 0}));
    EXPECT_FALSE(doc.isFabricPointBlocked(Canvas::FabricCoord{3, 0}));

    EXPECT_FALSE(doc.isFabricPointBlocked(Canvas::FabricCoord{4, 0}));
    EXPECT_TRUE(doc.isFabricPointBlocked(Canvas::FabricCoord{5, 0}));
    EXPECT_TRUE(doc.isFabricPointBlocked(Canvas::FabricCoord{6, 0}));
}
