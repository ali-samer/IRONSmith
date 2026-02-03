#include <gtest/gtest.h>

#include <canvas/CanvasSceneModel.hpp>

#include <designmodel/DesignDocument.hpp>
#include <designmodel/DesignMetadata.hpp>
#include <designmodel/DesignSchemaVersion.hpp>

using namespace Canvas;
using namespace DesignModel;

static DesignDocument emptyDoc()
{
    DesignMetadata md = DesignMetadata::createNew("D", "T", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);
    return b.freeze();
}

TEST(CanvasSceneModel, BuildsDeterministicAieTileRects)
{
    CanvasSceneModel sm;
    GridSpec spec;
    spec.aieCols = 4;
    spec.aieRows = 3;
    spec.memRows = 1;
    spec.shimRows = 1;
    sm.setGridSpec(spec);

    CanvasViewport vp; // default zoom=1.0
    CanvasRenderOptions opts;

    sm.rebuild(emptyDoc(), vp, opts);

    const int expected = spec.aieCols * spec.aieRows + spec.aieCols * spec.memRows + spec.aieCols * spec.shimRows;
    EXPECT_EQ(sm.tiles().size(), expected);

    const QRectF r0 = sm.computeTileRect(TileCoord(0, 0));
    const QRectF r1 = sm.computeTileRect(TileCoord(1, 0));
    ASSERT_FALSE(r0.isEmpty());
    ASSERT_FALSE(r1.isEmpty());
    EXPECT_GT(r0.top(), r1.top());
}

TEST(CanvasSceneModel, FabricOverlayFollowsRenderOptionsAndZoom)
{
    CanvasSceneModel sm;
    GridSpec spec;
    spec.aieCols = 4;
    spec.aieRows = 3;
    spec.memRows = 1;
    spec.shimRows = 1;
    sm.setGridSpec(spec);

    CanvasRenderOptions opts;
    opts.showFabric = false;

    CanvasViewport vp1; // zoom = 1.0
    sm.rebuild(emptyDoc(), vp1, opts);
    EXPECT_TRUE(sm.fabricNodes().isEmpty());
    EXPECT_TRUE(sm.fabricEdges().isEmpty());

    opts.showFabric = true;
    sm.rebuild(emptyDoc(), vp1, opts);
    ASSERT_FALSE(sm.fabricNodes().isEmpty());
    ASSERT_FALSE(sm.fabricEdges().isEmpty());
    const int nodeCount = sm.fabricNodes().size();
    const QPointF n1 = sm.fabricNodes().front().pos;

    CanvasViewport vp2;
    vp2.setZoomIndex(5); // 2.0
    sm.rebuild(emptyDoc(), vp2, opts);
    ASSERT_EQ(sm.fabricNodes().size(), nodeCount);
    const QPointF n2 = sm.fabricNodes().front().pos;

    EXPECT_NEAR(n2.x(), n1.x() * 2.0, 1e-3);
    EXPECT_NEAR(n2.y(), n1.y() * 2.0, 1e-3);
}