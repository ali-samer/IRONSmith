#include <gtest/gtest.h>

#include "designmodel/DesignDocument.hpp"

using namespace DesignModel;

TEST(DesignIndex, PortsLinksAndOccupancy) {
	DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
	DesignDocument::Builder b(DesignSchemaVersion::current(), md);

	const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(0, 0)), "A");
	const BlockId m = b.createBlock(BlockType::Memory, Placement(TileCoord(0, 1)), "M");

	const PortId aOut = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
	const PortId mIn  = b.createPort(m, PortDirection::Input,  PortType(PortTypeKind::Stream), "in");

	const LinkId l = b.createLink(aOut, mIn, "A->M");

	const DesignDocument doc = b.freeze();
	ASSERT_TRUE(doc.isValid());

	const auto& idx = doc.index();
	ASSERT_FALSE(idx.isEmpty());

	EXPECT_EQ(idx.portsForBlock(a).size(), 1);
	EXPECT_EQ(idx.portsForBlock(m).size(), 1);

	EXPECT_EQ(idx.linksForPort(aOut).size(), 1);
	EXPECT_EQ(idx.linksForPort(mIn).size(), 1);
	EXPECT_EQ(idx.linksForPort(aOut)[0], l);

	EXPECT_EQ(idx.blockAtTile(TileCoord(0, 0)), a);
	EXPECT_EQ(idx.blockAtTile(TileCoord(0, 1)), m);
	EXPECT_TRUE(idx.collidingTiles().isEmpty());
}

TEST(DesignIndex, DetectsCollisions) {
	DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
	DesignDocument::Builder b(DesignSchemaVersion::current(), md);

	const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 1), 2, 2), "A");
	const BlockId b2 = b.createBlock(BlockType::Memory, Placement(TileCoord(2, 2), 1, 1), "B");

	const DesignDocument doc = b.freeze();
	ASSERT_TRUE(doc.isValid());

	const auto& idx = doc.index();
	ASSERT_FALSE(idx.collidingTiles().isEmpty());
	EXPECT_EQ(idx.collidingTiles()[0], TileCoord(2, 2));

	EXPECT_NE(idx.blockAtTile(TileCoord(2, 2)).isNull(), true);
}