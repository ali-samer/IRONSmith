#include "utils/ui/GridLayout.hpp"

#include <gtest/gtest.h>

using Utils::GridLayout;
using Utils::GridOrigin;
using Utils::GridRect;
using Utils::GridSpec;

TEST(GridLayoutTests, ResolveCellSizeUsesExplicitSize)
{
    GridSpec spec;
    spec.columns = 4;
    spec.rows = 3;
    spec.autoCellSize = false;
    spec.cellSize = QSizeF(48.0, 32.0);

    const QSizeF size = GridLayout::resolveCellSize(spec, QSizeF(800.0, 600.0), 100.0);
    EXPECT_DOUBLE_EQ(size.width(), 48.0);
    EXPECT_DOUBLE_EQ(size.height(), 32.0);
}

TEST(GridLayoutTests, ResolveCellSizeFitsViewport)
{
    GridSpec spec;
    spec.columns = 4;
    spec.rows = 2;
    spec.autoCellSize = true;
    spec.cellSpacing = QSizeF(10.0, 10.0);
    spec.outerMargin = QMarginsF(20.0, 20.0, 20.0, 20.0);

    const QSizeF size = GridLayout::resolveCellSize(spec, QSizeF(500.0, 300.0), 50.0);
    EXPECT_GT(size.width(), 0.0);
    EXPECT_DOUBLE_EQ(size.width(), size.height());
}

TEST(GridLayoutTests, RectForGridBottomLeft)
{
    GridSpec spec;
    spec.columns = 4;
    spec.rows = 4;
    spec.origin = GridOrigin::BottomLeft;
    spec.cellSpacing = QSizeF(10.0, 5.0);
    spec.outerMargin = QMarginsF(2.0, 3.0, 4.0, 6.0);

    GridRect rect;
    rect.column = 1;
    rect.row = 2;
    rect.columnSpan = 2;
    rect.rowSpan = 1;

    const QRectF out = GridLayout::rectForGrid(spec, rect, QSizeF(20.0, 10.0));
    EXPECT_DOUBLE_EQ(out.left(), 2.0 + 1 * (20.0 + 10.0));
    EXPECT_DOUBLE_EQ(out.top(), 3.0 + (spec.rows - rect.row - rect.rowSpan) * (10.0 + 5.0));
    EXPECT_DOUBLE_EQ(out.width(), 2 * 20.0 + 1 * 10.0);
    EXPECT_DOUBLE_EQ(out.height(), 10.0);
}

TEST(GridLayoutTests, RectForGridTopLeft)
{
    GridSpec spec;
    spec.columns = 3;
    spec.rows = 3;
    spec.origin = GridOrigin::TopLeft;
    spec.cellSpacing = QSizeF(4.0, 4.0);
    spec.outerMargin = QMarginsF(1.0, 2.0, 1.0, 2.0);

    GridRect rect;
    rect.column = 0;
    rect.row = 0;
    rect.columnSpan = 1;
    rect.rowSpan = 1;

    const QRectF out = GridLayout::rectForGrid(spec, rect, QSizeF(10.0, 10.0));
    EXPECT_DOUBLE_EQ(out.left(), 1.0);
    EXPECT_DOUBLE_EQ(out.top(), 2.0 + rect.row * (10.0 + 4.0));
    EXPECT_DOUBLE_EQ(out.width(), 10.0);
    EXPECT_DOUBLE_EQ(out.height(), 10.0);
}
