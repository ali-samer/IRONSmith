#include <gtest/gtest.h>

#include "command/DeleteCommands.hpp"
#include "command/BuiltInCommands.hpp"

#include "designmodel/DesignDocument.hpp"

using namespace DesignModel;
using namespace Command;

static DesignDocument makeDocWithTwoBlocksOneLink(BlockId* aBlock = nullptr, BlockId* bBlock = nullptr,
                                                  PortId* aOut = nullptr, PortId* bIn = nullptr,
                                                  LinkId* link = nullptr)
{
    DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 1)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 2)), "B");

    const PortId out = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out", 4);
    const PortId in = b.createPort(c, PortDirection::Input, PortType(PortTypeKind::Stream), "in", 1);

    const LinkId l = b.createLink(out, in);

    if (aBlock) *aBlock = a;
    if (bBlock) *bBlock = c;
    if (aOut) *aOut = out;
    if (bIn) *bIn = in;
    if (link) *link = l;

    return b.freeze();
}

TEST(DeleteEntitiesCommand, DeletesLink) {
    LinkId l;
    const DesignDocument doc = makeDocWithTwoBlocksOneLink(nullptr, nullptr, nullptr, nullptr, &l);
    ASSERT_TRUE(doc.isValid());

    DeleteEntitiesCommand cmd({}, {l}, {}, {}, {});
    const auto r = cmd.apply(doc);
    ASSERT_TRUE(r.ok());
    ASSERT_TRUE(r.document().isValid());
    EXPECT_EQ(r.document().tryLink(l), nullptr);
    EXPECT_EQ(r.document().linkIds().size(), 0);
}

TEST(DeleteEntitiesCommand, DeletesBlockCascades) {
    BlockId a;
    LinkId l;
    const DesignDocument doc = makeDocWithTwoBlocksOneLink(&a, nullptr, nullptr, nullptr, &l);
    ASSERT_TRUE(doc.isValid());

    DeleteEntitiesCommand cmd({a}, {}, {}, {}, {});
    const auto r = cmd.apply(doc);
    ASSERT_TRUE(r.ok());
    ASSERT_TRUE(r.document().isValid());
    EXPECT_EQ(r.document().tryBlock(a), nullptr);
    EXPECT_EQ(r.document().tryLink(l), nullptr);
}

TEST(CreateLinkCommand, RejectsDirectionMismatch) {
    DesignMetadata md = DesignMetadata::createNew("Design", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(2, 2)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(2, 3)), "B");

    const PortId aIn = b.createPort(a, PortDirection::Input, PortType(PortTypeKind::Stream), "in");
    const PortId cIn = b.createPort(c, PortDirection::Input, PortType(PortTypeKind::Stream), "in");

    const DesignDocument doc = b.freeze();
    ASSERT_TRUE(doc.isValid());

    CreateLinkCommand cmd(aIn, cIn);
    const auto r = cmd.apply(doc);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), CommandErrorCode::InvalidConnection);
}

TEST(CreateLinkCommand, EnforcesInputCapacity) {
    BlockId a;
    BlockId c;
    PortId out;
    PortId in;
    LinkId l;
    const DesignDocument doc = makeDocWithTwoBlocksOneLink(&a, &c, &out, &in, &l);
    ASSERT_TRUE(doc.isValid());

    DesignDocument::Builder b(doc);
    const PortId out2 = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out2");
    const DesignDocument doc2 = b.freeze();
    ASSERT_TRUE(doc2.isValid());

    CreateLinkCommand cmd(out2, in);
    const auto r = cmd.apply(doc2);
    ASSERT_FALSE(r.ok());
    EXPECT_EQ(r.error().code(), CommandErrorCode::InvalidConnection);
}
