#include <gtest/gtest.h>

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasCommands.hpp"

TEST(CanvasCommandTests, MoveCommandUndoRedo)
{
    Canvas::CanvasDocument doc;

    auto* blk = doc.createBlock(QRectF(0.0, 0.0, 40.0, 20.0), true);
    ASSERT_NE(blk, nullptr);

    const Canvas::ObjectId id = blk->id();
    const QPointF start = blk->boundsScene().topLeft();
    const QPointF end(64.0, 32.0);

    ASSERT_TRUE(doc.previewSetItemTopLeft(id, end));
    EXPECT_EQ(blk->boundsScene().topLeft(), end);

    ASSERT_TRUE(doc.commands().execute(std::make_unique<Canvas::MoveItemCommand>(id, start, end)));
    EXPECT_TRUE(doc.commands().canUndo());
    EXPECT_FALSE(doc.commands().canRedo());
    EXPECT_EQ(blk->boundsScene().topLeft(), end);

    ASSERT_TRUE(doc.commands().undo());
    EXPECT_EQ(blk->boundsScene().topLeft(), start);
    EXPECT_FALSE(doc.commands().canUndo());
    EXPECT_TRUE(doc.commands().canRedo());

    ASSERT_TRUE(doc.commands().redo());
    EXPECT_EQ(blk->boundsScene().topLeft(), end);
    EXPECT_TRUE(doc.commands().canUndo());
    EXPECT_FALSE(doc.commands().canRedo());
}

TEST(CanvasCommandTests, FixedBlockCannotMove)
{
    Canvas::CanvasDocument doc;
    auto* blk = doc.createBlock(QRectF(0.0, 0.0, 40.0, 20.0), false);
    ASSERT_NE(blk, nullptr);

    const Canvas::ObjectId id = blk->id();
    const QPointF start = blk->boundsScene().topLeft();
    const QPointF end(64.0, 32.0);

    EXPECT_FALSE(doc.previewSetItemTopLeft(id, end));
    EXPECT_EQ(blk->boundsScene().topLeft(), start);

    EXPECT_FALSE(doc.commands().execute(std::make_unique<Canvas::MoveItemCommand>(id, start, end)));
    EXPECT_FALSE(doc.commands().canUndo());
    EXPECT_FALSE(doc.commands().canRedo());
    EXPECT_EQ(blk->boundsScene().topLeft(), start);
}