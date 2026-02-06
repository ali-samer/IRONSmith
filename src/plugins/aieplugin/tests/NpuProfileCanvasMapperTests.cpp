#include <gtest/gtest.h>

#include <algorithm>

#include "aieplugin/NpuProfileCanvasMapper.hpp"

using Aie::CanvasGridModel;
using Aie::NpuProfile;

namespace {

Aie::TileGroup makeGroup(const QVector<int>& rows, const QVector<int>& cols)
{
    Aie::TileGroup group;
    group.rows = rows;
    group.cols = cols;
    return group;
}

} // namespace

TEST(NpuProfileCanvasMapperTests, BuildsGridAndBlocks)
{
    NpuProfile profile;
    profile.id = QStringLiteral("test");
    profile.grid.columns = 3;
    profile.grid.rows.shim = 1;
    profile.grid.rows.mem = 1;
    profile.grid.rows.aie = 1;

    profile.tiles.coordinateSystem = QStringLiteral("col_row");
    profile.tiles.shim = makeGroup({0}, {0, 1, 2});
    profile.tiles.mem = makeGroup({1}, {0, 1});
    profile.tiles.aie = makeGroup({2}, {1, 2});

    CanvasGridModel model;
    const Utils::Result result = Aie::buildCanvasGridModel(profile, model);
    ASSERT_TRUE(result.ok) << result.errors.join("\n").toStdString();

    EXPECT_EQ(model.gridSpec.columns, 3);
    EXPECT_EQ(model.gridSpec.rows, 3);
    EXPECT_EQ(model.blocks.size(), 7);

    auto hasId = [&](const QString& id) {
        return std::any_of(model.blocks.begin(), model.blocks.end(), [&](const auto& spec) {
            return spec.id == id;
        });
    };

    EXPECT_TRUE(hasId(QStringLiteral("shim0_0")));
    EXPECT_TRUE(hasId(QStringLiteral("mem1_1")));
    EXPECT_TRUE(hasId(QStringLiteral("aie2_2")));
}
