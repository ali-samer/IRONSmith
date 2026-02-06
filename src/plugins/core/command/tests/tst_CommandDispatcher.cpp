#include <gtest/gtest.h>

#include <QtTest/QSignalSpy>

#include "command/CommandDispatcher.hpp"
#include "command/BuiltInCommands.hpp"

#include <designmodel/DesignDocument.hpp>

using namespace DesignModel;
using namespace Command;

static DesignDocument makeEmptyDoc()
{
    DesignMetadata md = DesignMetadata::createNew("D", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);
    return b.freeze();
}

TEST(CommandDispatcher, ApplyCreateBlockEmitsSignalsAndUpdatesDoc)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    QSignalSpy docSpy(&d, &CommandDispatcher::documentChanged);
    QSignalSpy appliedSpy(&d, &CommandDispatcher::commandApplied);

    const CreateBlockCommand cmd(BlockType::Compute, Placement(TileCoord(0,0)), "A");
    const auto r = d.apply(cmd);

    ASSERT_TRUE(r.ok());
    EXPECT_EQ(docSpy.count(), 1);
    EXPECT_EQ(appliedSpy.count(), 1);
    EXPECT_EQ(d.document().blockIds().size(), 1);

    const CreatedBlock payload = r.payload().value<CreatedBlock>();
    EXPECT_FALSE(payload.id.isNull());
    EXPECT_NE(d.document().tryBlock(payload.id), nullptr);
}

TEST(CommandDispatcher, UndoRedoWorks)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    const auto r1 = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0,0)), "A"));
    ASSERT_TRUE(r1.ok());
    EXPECT_TRUE(d.canUndo());
    EXPECT_FALSE(d.canRedo());
    EXPECT_EQ(d.document().blockIds().size(), 1);

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    EXPECT_FALSE(d.canUndo());
    EXPECT_TRUE(d.canRedo());
    EXPECT_EQ(d.document().blockIds().size(), 0);

    const auto rr = d.redo();
    ASSERT_TRUE(rr.ok());
    EXPECT_TRUE(d.canUndo());
    EXPECT_FALSE(d.canRedo());
    EXPECT_EQ(d.document().blockIds().size(), 1);
}

TEST(CommandDispatcher, TransactionGroupsUndo)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    d.beginTransaction("Place two blocks");
    const auto r1 = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0,0)), "A"));
    const auto r2 = d.apply(CreateBlockCommand(BlockType::Memory, Placement(TileCoord(0,1)), "M"));
    ASSERT_TRUE(r1.ok());
    ASSERT_TRUE(r2.ok());
    d.commitTransaction();

    EXPECT_EQ(d.document().blockIds().size(), 2);
    EXPECT_TRUE(d.canUndo());

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    EXPECT_EQ(d.document().blockIds().size(), 0);
}

TEST(CommandDispatcher, CreatePortAndLink)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    const auto rb = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0,0)), "A"));
    ASSERT_TRUE(rb.ok());
    const auto bid = rb.payload().value<CreatedBlock>().id;

    const auto ro = d.apply(CreatePortCommand(bid, PortDirection::Output, PortType(PortTypeKind::Stream), "out"));
    const auto ri = d.apply(CreatePortCommand(bid, PortDirection::Input,  PortType(PortTypeKind::Stream), "in"));
    ASSERT_TRUE(ro.ok());
    ASSERT_TRUE(ri.ok());

    const auto outId = ro.payload().value<CreatedPort>().id;
    const auto inId  = ri.payload().value<CreatedPort>().id;

    const auto rl = d.apply(CreateLinkCommand(outId, inId, "A->A"));
    ASSERT_TRUE(rl.ok());

    EXPECT_EQ(d.document().portIds().size(), 2);
    EXPECT_EQ(d.document().linkIds().size(), 1);

    const auto lid = rl.payload().value<CreatedLink>().id;
    EXPECT_NE(d.document().tryLink(lid), nullptr);
}

TEST(CommandDispatcher, AdjustLinkRouteUndoRedo)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    const auto rb = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0,0)), "A"));
    ASSERT_TRUE(rb.ok());
    const auto bid = rb.payload().value<CreatedBlock>().id;

    const auto ro = d.apply(CreatePortCommand(bid, PortDirection::Output, PortType(PortTypeKind::Stream), "out"));
    const auto ri = d.apply(CreatePortCommand(bid, PortDirection::Input,  PortType(PortTypeKind::Stream), "in"));
    ASSERT_TRUE(ro.ok());
    ASSERT_TRUE(ri.ok());

    const auto outId = ro.payload().value<CreatedPort>().id;
    const auto inId  = ri.payload().value<CreatedPort>().id;

    const auto rl = d.apply(CreateLinkCommand(outId, inId, "A->A"));
    ASSERT_TRUE(rl.ok());
    const auto lid = rl.payload().value<CreatedLink>().id;

    const auto* link0 = d.document().tryLink(lid);
    ASSERT_NE(link0, nullptr);
    EXPECT_FALSE(link0->hasRouteOverride());

    QVector<QPointF> wp;
    wp.push_back(QPointF(10.0, 20.0));
    wp.push_back(QPointF(10.0, 40.0));
    const RouteOverride ov(std::move(wp), true);

    const auto r1 = d.apply(AdjustLinkRouteCommand(lid, std::nullopt, ov));
    ASSERT_TRUE(r1.ok());

    const auto* link1 = d.document().tryLink(lid);
    ASSERT_NE(link1, nullptr);
    ASSERT_TRUE(link1->hasRouteOverride());
    EXPECT_EQ(link1->routeOverride(), std::optional<RouteOverride>(ov));

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    const auto* link2 = d.document().tryLink(lid);
    ASSERT_NE(link2, nullptr);
    EXPECT_FALSE(link2->hasRouteOverride());

    const auto rr = d.redo();
    ASSERT_TRUE(rr.ok());
    const auto* link3 = d.document().tryLink(lid);
    ASSERT_NE(link3, nullptr);
    ASSERT_TRUE(link3->hasRouteOverride());
    EXPECT_EQ(link3->routeOverride(), std::optional<RouteOverride>(ov));
}
