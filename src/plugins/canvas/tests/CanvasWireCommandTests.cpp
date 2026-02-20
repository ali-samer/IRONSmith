// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/CanvasCommands.hpp"
#include "canvas/CanvasDocument.hpp"
#include "canvas/CanvasWire.hpp"
#include "canvas/CanvasBlock.hpp"
#include "canvas/CanvasPorts.hpp"

#include <QtCore/QPointF>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtCore/QString>

namespace Canvas {

TEST(CanvasWireCommandTests, CreateUndoRedoRestoresSameWireId)
{
    CanvasDocument doc;
    auto* blk = doc.createBlock(QRectF(QPointF(64.0, 64.0), QSizeF(160.0, 96.0)), false);
    ASSERT_NE(blk, nullptr);

    const PortId portA = PortId::create();
    const PortId portB = PortId::create();

    std::vector<CanvasPort> ports;
    ports.push_back(CanvasPort{portA, PortRole::Dynamic, PortSide::Left, 0.50, QStringLiteral("D0")});
    ports.push_back(CanvasPort{portB, PortRole::Dynamic, PortSide::Right, 0.25, QStringLiteral("D1")});
    blk->setPorts(std::move(ports));

    CanvasWire::Endpoint a;
    a.attached = PortRef{blk->id(), portA};
    a.freeScene = blk->portAnchorScene(portA);
    CanvasWire::Endpoint b;
    b.attached = PortRef{blk->id(), portB};
    b.freeScene = blk->portAnchorScene(portB);

    auto w = std::make_unique<CanvasWire>(a, b);
    const ObjectId wid = doc.allocateId();
    w->setId(wid);

    ASSERT_TRUE(doc.commands().execute(std::make_unique<CreateItemCommand>(std::move(w))));
    ASSERT_EQ(doc.items().size(), 2u);

    bool found = false;
    for (const auto& it : doc.items()) {
        if (it && it->id() == wid)
            found = true;
    }
    EXPECT_TRUE(found);

    ASSERT_TRUE(doc.commands().undo());

    ASSERT_EQ(doc.items().size(), 1u);
    EXPECT_EQ(doc.items()[0]->id(), blk->id());

    ASSERT_TRUE(doc.commands().redo());
    ASSERT_EQ(doc.items().size(), 2u);

    found = false;
    for (const auto& it : doc.items()) {
        if (it && it->id() == wid)
            found = true;
    }
    EXPECT_TRUE(found);
}

} // namespace Canvas
