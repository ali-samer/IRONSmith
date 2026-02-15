// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasCommands.hpp"

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
