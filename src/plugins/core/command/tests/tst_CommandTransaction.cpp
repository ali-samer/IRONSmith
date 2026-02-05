#include <gtest/gtest.h>

#include <QtTest/QSignalSpy>

#include "command/CommandTransaction.hpp"
#include "command/BuiltInCommands.hpp"
#include "command/CommandDispatcher.hpp"

#include <designmodel/DesignDocument.hpp>

using namespace DesignModel;
using namespace Command;

static DesignDocument makeEmptyDoc()
{
    DesignMetadata md = DesignMetadata::createNew("D", "Joe", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);
    return b.freeze();
}

TEST(CommandTransaction, DestructorCommitsByDefault)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    {
        CommandTransaction tx(d, "tx");
        const auto r1 = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0, 0)), "A"));
        const auto r2 = d.apply(CreateBlockCommand(BlockType::Memory,  Placement(TileCoord(0, 1)), "M"));
        ASSERT_TRUE(r1.ok());
        ASSERT_TRUE(r2.ok());
        EXPECT_EQ(d.document().blockIds().size(), 2);
        EXPECT_FALSE(d.canUndo());
    }

    EXPECT_TRUE(d.canUndo());
    EXPECT_EQ(d.document().blockIds().size(), 2);

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    EXPECT_EQ(d.document().blockIds().size(), 0);
}

TEST(CommandTransaction, ExplicitRollbackRestoresPreTxSnapshot)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    ASSERT_EQ(d.document().blockIds().size(), 0);

    {
        CommandTransaction tx(d, "tx");
        const auto r = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0, 0)), "A"));
        ASSERT_TRUE(r.ok());
        EXPECT_EQ(d.document().blockIds().size(), 1);

        tx.rollback();
        EXPECT_EQ(d.document().blockIds().size(), 0);
        EXPECT_FALSE(d.canUndo());
        EXPECT_FALSE(d.canRedo());
    }

    EXPECT_EQ(d.document().blockIds().size(), 0);
    EXPECT_FALSE(d.canUndo());
}

TEST(CommandTransaction, ExplicitCommitGroupsUndo)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    {
        CommandTransaction tx(d, "tx");
        const auto r1 = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0, 0)), "A"));
        const auto r2 = d.apply(CreateBlockCommand(BlockType::Memory,  Placement(TileCoord(0, 1)), "M"));
        ASSERT_TRUE(r1.ok());
        ASSERT_TRUE(r2.ok());

        tx.commit();
        EXPECT_TRUE(d.canUndo());
        EXPECT_EQ(d.document().blockIds().size(), 2);
    }

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    EXPECT_EQ(d.document().blockIds().size(), 0);
}

TEST(CommandTransaction, RedoClearedOnMutation)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    const auto r1 = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0, 0)), "A"));
    ASSERT_TRUE(r1.ok());
    ASSERT_TRUE(d.canUndo());

    const auto u = d.undo();
    ASSERT_TRUE(u.ok());
    ASSERT_TRUE(d.canRedo());

    const auto r2 = d.apply(CreateBlockCommand(BlockType::Memory, Placement(TileCoord(0, 1)), "M"));
    ASSERT_TRUE(r2.ok());
    EXPECT_FALSE(d.canRedo());
}

TEST(CommandTransaction, UndoRedoDisallowedDuringTransaction)
{
    CommandDispatcher d;
    d.setDocument(makeEmptyDoc());

    CommandTransaction tx(d, "tx");
    const auto r = d.apply(CreateBlockCommand(BlockType::Compute, Placement(TileCoord(0, 0)), "A"));
    ASSERT_TRUE(r.ok());

    const auto u = d.undo();
    EXPECT_FALSE(u.ok());

    const auto rr = d.redo();
    EXPECT_FALSE(rr.ok());

    tx.rollback();
    EXPECT_EQ(d.document().blockIds().size(), 0);
}