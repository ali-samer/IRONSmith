#include <gtest/gtest.h>

#include "designmodel/DesignDocument.hpp"

using namespace DesignModel;

TEST(DesignDocument, RemoveLinkMaintainsValidity) {
    DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 1)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 2)), "B");

    const PortId aOut = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
    const PortId cIn = b.createPort(c, PortDirection::Input, PortType(PortTypeKind::Stream), "in");

    const LinkId l = b.createLink(aOut, cIn);

    const DesignDocument doc = b.freeze();
    ASSERT_TRUE(doc.isValid());

    DesignDocument::Builder b2(doc);
    EXPECT_TRUE(b2.removeLink(l));

    const DesignDocument out = b2.freeze();
    ASSERT_TRUE(out.isValid());
    EXPECT_EQ(out.tryLink(l), nullptr);
    EXPECT_EQ(out.linkIds().size(), 0);
}

TEST(DesignDocument, RemoveBlockCascadesPortsAndLinks) {
    DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(2, 2)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(2, 3)), "B");

    const PortId aOut = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
    const PortId aIn = b.createPort(a, PortDirection::Input, PortType(PortTypeKind::Stream), "in");
    const PortId cIn = b.createPort(c, PortDirection::Input, PortType(PortTypeKind::Stream), "in");

    const LinkId l1 = b.createLink(aOut, cIn);
    const LinkId l2 = b.createLink(aOut, aIn);

    const DesignDocument doc = b.freeze();
    ASSERT_TRUE(doc.isValid());
    ASSERT_EQ(doc.blockIds().size(), 2);
    ASSERT_EQ(doc.linkIds().size(), 2);

    DesignDocument::Builder b2(doc);
    EXPECT_TRUE(b2.removeBlock(a));

    const DesignDocument out = b2.freeze();
    ASSERT_TRUE(out.isValid());

    EXPECT_EQ(out.tryBlock(a), nullptr);
    EXPECT_EQ(out.tryPort(aOut), nullptr);
    EXPECT_EQ(out.tryPort(aIn), nullptr);

    EXPECT_EQ(out.tryLink(l1), nullptr);
    EXPECT_EQ(out.tryLink(l2), nullptr);

    EXPECT_NE(out.tryBlock(c), nullptr);
    EXPECT_NE(out.tryPort(cIn), nullptr);

    EXPECT_EQ(out.blockIds().size(), 1);
    EXPECT_EQ(out.linkIds().size(), 0);
}


TEST(DesignDocument, SetLinkRouteOverride) {
    DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 1)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 2)), "B");

    const PortId aOut = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
    const PortId cIn = b.createPort(c, PortDirection::Input, PortType(PortTypeKind::Stream), "in");

    const LinkId l = b.createLink(aOut, cIn);

    RouteOverride ov(QVector<QPointF>{ QPointF(40.0, 10.0), QPointF(40.0, 60.0) }, true);
    ASSERT_TRUE(ov.isValid());

    EXPECT_TRUE(b.setLinkRouteOverride(l, ov));
    EXPECT_FALSE(b.setLinkRouteOverride(l, ov)); // no-op

    const DesignDocument doc = b.freeze();
    const Link* link = doc.tryLink(l);
    ASSERT_NE(link, nullptr);
    ASSERT_TRUE(link->hasRouteOverride());
    ASSERT_TRUE(link->routeOverride().has_value());
    EXPECT_EQ(link->routeOverride()->waypointsWorld().size(), 2);
    EXPECT_EQ(link->routeOverride()->waypointsWorld().at(0), QPointF(40.0, 10.0));

    DesignDocument::Builder b2(doc);
    EXPECT_TRUE(b2.setLinkRouteOverride(l, std::nullopt));
    const DesignDocument doc2 = b2.freeze();

    const Link* link2 = doc2.tryLink(l);
    ASSERT_NE(link2, nullptr);
    EXPECT_FALSE(link2->hasRouteOverride());
}