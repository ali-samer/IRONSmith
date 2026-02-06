#include <gtest/gtest.h>

#include "aieplugin/NpuProfileLoader.hpp"

using Aie::NpuProfileCatalog;

TEST(NpuProfileLoaderTests, ParsesMinimalCatalog)
{
    const QByteArray json = R"JSON(
{
  "schemaVersion": 1,
  "devices": [
    {
      "id": "dev1",
      "name": "Device",
      "vendor": "AMD",
      "family": "XDNA",
      "grid": {
        "columns": 2,
        "rows": { "shim": 1, "mem": 1, "aie": 0 },
        "rowOrderBottomToTop": ["shim", "mem"]
      },
      "tiles": {
        "coordinateSystem": "col_row",
        "shim": { "rows": [0], "cols": [0, 1], "virtualCols": [] },
        "mem": { "rows": [1], "cols": [0, 1] },
        "aie": { "rows": [], "cols": [] }
      }
    }
  ]
}
)JSON";

    NpuProfileCatalog catalog;
    const Utils::Result result = Aie::loadProfileCatalogFromJson(json, catalog);
    ASSERT_TRUE(result.ok) << result.errors.join("\n").toStdString();

    ASSERT_EQ(catalog.devices.size(), 1);
    const auto& profile = catalog.devices.front();
    EXPECT_EQ(profile.id, QStringLiteral("dev1"));
    EXPECT_EQ(profile.grid.columns, 2);
    EXPECT_EQ(profile.grid.rows.total(), 2);
    EXPECT_EQ(profile.tiles.coordinateSystem, QStringLiteral("col_row"));
}
