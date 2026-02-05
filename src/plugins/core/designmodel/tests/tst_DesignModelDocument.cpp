#include <gtest/gtest.h>

#include "designmodel/DesignDocument.hpp"

using namespace DesignModel;

TEST(DesignDocument, BuilderCreatesValidSnapshot) {
	DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
	DesignDocument::Builder b(DesignSchemaVersion::current(), md);

	const BlockId blk = b.createBlock(BlockType::Compute, Placement(TileCoord(2, 3)), "AIE0");
	const PortId out = b.createPort(blk, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
	const PortId in  = b.createPort(blk, PortDirection::Input,  PortType(PortTypeKind::Stream), "in");

	const LinkId link = b.createLink(out, in, "loop");

	const DesignDocument doc = b.freeze();
	ASSERT_TRUE(doc.isValid());

	ASSERT_NE(doc.tryBlock(blk), nullptr);
	ASSERT_NE(doc.tryPort(out), nullptr);
	ASSERT_NE(doc.tryPort(in), nullptr);
	ASSERT_NE(doc.tryLink(link), nullptr);

	EXPECT_EQ(doc.blockIds().size(), 1);
	EXPECT_EQ(doc.portIds().size(), 2);
	EXPECT_EQ(doc.linkIds().size(), 1);

	const Block* block = doc.tryBlock(blk);
	ASSERT_NE(block, nullptr);
	EXPECT_EQ(block->ports().size(), 2);
	EXPECT_EQ(block->ports()[0], out);
	EXPECT_EQ(block->ports()[1], in);
}

TEST(DesignDocument, EmptyIsInvalid) {
	DesignDocument doc;
	EXPECT_FALSE(doc.isValid());
	EXPECT_EQ(doc.blockIds().size(), 0);
	EXPECT_EQ(doc.portIds().size(), 0);
	EXPECT_EQ(doc.linkIds().size(), 0);
}