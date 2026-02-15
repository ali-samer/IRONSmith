// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasCommands.hpp"
#include "canvas/CanvasBlock.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

namespace Canvas {

TEST(CanvasDeleteCommandTests, DeleteUndoRedoRestoresSameId)
{
    CanvasDocument doc;

    auto* blk = doc.createBlock(QRectF(QPointF(10.0, 20.0), QSizeF(100.0, 50.0)), false);
    ASSERT_NE(blk, nullptr);
    const ObjectId id = blk->id();

    ASSERT_EQ(doc.items().size(), 1u);

    ASSERT_TRUE(doc.commands().execute(std::make_unique<DeleteItemCommand>(id)));
    EXPECT_EQ(doc.items().size(), 0u);

    ASSERT_TRUE(doc.commands().undo());
    ASSERT_EQ(doc.items().size(), 1u);
    EXPECT_EQ(doc.items()[0]->id(), id);

    ASSERT_TRUE(doc.commands().redo());
    EXPECT_EQ(doc.items().size(), 0u);
}

TEST(CanvasDeleteCommandTests, DeleteFailsForMissingId)
{
    CanvasDocument doc;
    EXPECT_FALSE(doc.commands().execute(std::make_unique<DeleteItemCommand>(ObjectId{})));
}

} // namespace Canvas
