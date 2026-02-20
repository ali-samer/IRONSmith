// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/CanvasFabric.hpp"

namespace {

bool containsCoord(const std::vector<Canvas::FabricCoord>& v, int x, int y)
{
    for (const auto& c : v) {
        if (c.x == x && c.y == y)
            return true;
    }
    return false;
}

struct BlockSet {
    std::vector<Canvas::FabricCoord> coords;
};

bool isBlocked(const Canvas::FabricCoord& c, void* user)
{
    auto* set = static_cast<BlockSet*>(user);
    if (!set)
        return false;
    for (const auto& b : set->coords) {
        if (b.x == c.x && b.y == c.y)
            return true;
    }
    return false;
}

} // namespace

TEST(CanvasFabricTests, EnumerateVisibleRectExpectedCount)
{
    Canvas::CanvasFabric::Config cfg;
    cfg.step = 10.0;
    Canvas::CanvasFabric fabric(cfg);

    const QRectF rect(QPointF(0.0, 0.0), QPointF(20.0, 20.0));
    const auto coords = fabric.enumerate(rect);

    EXPECT_EQ(static_cast<int>(coords.size()), 25);
    EXPECT_TRUE(containsCoord(coords, -1, -1));
    EXPECT_TRUE(containsCoord(coords, 0, 0));
    EXPECT_TRUE(containsCoord(coords, 3, 3));
    EXPECT_FALSE(containsCoord(coords, -2, 0));
    EXPECT_FALSE(containsCoord(coords, 4, 0));
}

TEST(CanvasFabricTests, EnumerateWithMaskFiltersPoints)
{
    Canvas::CanvasFabric::Config cfg;
    cfg.step = 10.0;
    Canvas::CanvasFabric fabric(cfg);

    BlockSet set;
    set.coords.push_back(Canvas::FabricCoord{0, 0});
    set.coords.push_back(Canvas::FabricCoord{1, 1});
    set.coords.push_back(Canvas::FabricCoord{3, 3});

    const QRectF rect(QPointF(0.0, 0.0), QPointF(20.0, 20.0));
    const auto coords = fabric.enumerate(rect, &isBlocked, &set);

    EXPECT_EQ(static_cast<int>(coords.size()), 22);
    EXPECT_FALSE(containsCoord(coords, 0, 0));
    EXPECT_FALSE(containsCoord(coords, 1, 1));
    EXPECT_FALSE(containsCoord(coords, 3, 3));
    EXPECT_TRUE(containsCoord(coords, -1, -1));
    EXPECT_TRUE(containsCoord(coords, 2, 2));
}
