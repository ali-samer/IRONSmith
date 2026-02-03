#include <gtest/gtest.h>

#define private public
#include <canvas/CanvasView.hpp>
#undef private

#include <command/CommandDispatcher.hpp>
#include <designmodel/DesignDocument.hpp>
#include <designmodel/DesignMetadata.hpp>
#include <designmodel/DesignSchemaVersion.hpp>

#include <QtWidgets/QApplication>

#include "canvas/CanvasLinkRouteEditor.hpp"

using namespace Canvas;
using namespace DesignModel;
using namespace Command;

namespace {

struct GuiAppGuard {
    GuiAppGuard() {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "CanvasTests";
            static char* argv[] = { arg0, nullptr };
            m_app = new QApplication(argc, argv);
        }
    }
    ~GuiAppGuard() = default;

    QApplication* m_app{nullptr};
};

DesignDocument makeDocWithOneLink()
{
    DesignMetadata md = DesignMetadata::createNew("D", "T", "profile:stub");
    DesignDocument::Builder b(DesignSchemaVersion::current(), md);

    const BlockId a = b.createBlock(BlockType::Compute, Placement(TileCoord(1, 1)), "A");
    const BlockId c = b.createBlock(BlockType::Compute, Placement(TileCoord(4, 6)), "B");

    const PortId aOut = b.createPort(a, PortDirection::Output, PortType(PortTypeKind::Stream), "out");
    const PortId cIn  = b.createPort(c, PortDirection::Input,  PortType(PortTypeKind::Stream), "in");

    (void)b.createLink(aOut, cIn);
    return b.freeze();
}

struct PickedSeg {
    LinkId link;
    int seg{-1};
    QPointF screenMid;
    QPointF worldMid;
};

PickedSeg pickEditableSegment(const CanvasView& v)
{
    const auto& links = v.m_scene.links();
    EXPECT_FALSE(links.isEmpty());

    const auto& lv = links.front();
    EXPECT_GE(lv.worldPoints.size(), 2);

    int best = -1;
    for (int i = 0; i + 1 < lv.worldPoints.size(); ++i) {
        const QPointF a = lv.worldPoints[i];
        const QPointF b = lv.worldPoints[i + 1];
        const bool axis = qFuzzyCompare(a.x(), b.x()) || qFuzzyCompare(a.y(), b.y());
        if (!axis)
            continue;
        best = i;
        break;
    }

    EXPECT_NE(best, -1);

    const QPointF wa = lv.worldPoints[best];
    const QPointF wb = lv.worldPoints[best + 1];
    const QPointF pa = lv.points[best];
    const QPointF pb = lv.points[best + 1];

    PickedSeg out;
    out.link = lv.id;
    out.seg = best;
    out.worldMid  = (wa + wb) * 0.5;
    out.screenMid = (pa + pb) * 0.5;
    return out;
}

QPointF findValidDragPointWorld(const CanvasView& v,
                               const QVector<QPointF>& baseWorld,
                               int seg,
                               QPointF worldMid)
{
    const QPointF a = baseWorld[seg];
    const QPointF b = baseWorld[seg + 1];

    const bool horizontal = qFuzzyCompare(a.y(), b.y());
    const double clearance = 2.0;

    if (horizontal) {
        const double curY = a.y();
        for (const double yLane : v.m_scene.fabricYs()) {
            if (qAbs(yLane - curY) < 1e-6)
                continue;
            const QPointF probe(worldMid.x(), yLane + 0.25);
            const auto r = LinkRouteEditor::shiftSegmentToNearestLane(
                baseWorld, seg, probe,
                v.m_scene.fabricXs(), v.m_scene.fabricYs(), v.m_scene.fabricObstacles(), clearance);
            if (r.ok && r.worldPoints != baseWorld)
                return probe;
        }
    } else {
        const double curX = a.x();
        for (const double xLane : v.m_scene.fabricXs()) {
            if (qAbs(xLane - curX) < 1e-6)
                continue;
            const QPointF probe(xLane + 0.25, worldMid.y());
            const auto r = LinkRouteEditor::shiftSegmentToNearestLane(
                baseWorld, seg, probe,
                v.m_scene.fabricXs(), v.m_scene.fabricYs(), v.m_scene.fabricObstacles(), clearance);
            if (r.ok && r.worldPoints != baseWorld)
                return probe;
        }
    }

    return QPointF();
}

} // namespace

TEST(CanvasRouteEdit, CommitAppliesAdjustLinkRouteCommand)
{
    GuiAppGuard app;

    CommandDispatcher dispatcher;
    dispatcher.setDocument(makeDocWithOneLink());

    CanvasView view;
    view.setCommandDispatcher(&dispatcher);
    view.setDocument(dispatcher.document());
    view.setMode(EditorModeKind::Linking);

    const auto picked = pickEditableSegment(view);

    ASSERT_TRUE(view.beginRouteEdit(picked.screenMid));

    const QPointF dragWorld = findValidDragPointWorld(view, view.m_routeEditCurrentWorld, picked.seg, picked.worldMid);
    ASSERT_FALSE(dragWorld.isNull());

    view.updateRouteEdit(view.m_vp.worldToScreen(dragWorld));
    ASSERT_TRUE(view.m_routeEditPreviewValid);
    ASSERT_NE(view.m_routeEditCurrentWorld, view.m_routeEditBaseWorld);

    const QVector<QPointF> expectedMid = view.m_routeEditCurrentWorld.mid(1, view.m_routeEditCurrentWorld.size() - 2);

    view.commitRouteEdit();

    const auto& out = dispatcher.document();
    const auto* link = out.tryLink(picked.link);
    ASSERT_NE(link, nullptr);
    ASSERT_TRUE(link->hasRouteOverride());

    const auto ov = link->routeOverride();
    ASSERT_TRUE(ov.has_value());
    EXPECT_EQ(ov->waypointsWorld(), expectedMid);
}

TEST(CanvasRouteEdit, CommitUsesLastKnownGoodWhenFinalPreviewIsInvalid)
{
    GuiAppGuard app;

    CommandDispatcher dispatcher;
    dispatcher.setDocument(makeDocWithOneLink());

    CanvasView view;
    view.setCommandDispatcher(&dispatcher);
    view.setDocument(dispatcher.document());
    view.setMode(EditorModeKind::Linking);

    const auto picked = pickEditableSegment(view);

    ASSERT_TRUE(view.beginRouteEdit(picked.screenMid));

    const QPointF dragWorld = findValidDragPointWorld(view, view.m_routeEditCurrentWorld, picked.seg, picked.worldMid);
    ASSERT_FALSE(dragWorld.isNull());

    view.updateRouteEdit(view.m_vp.worldToScreen(dragWorld));
    ASSERT_TRUE(view.m_routeEditPreviewValid);

    const QVector<QPointF> lastGood = view.m_routeEditCurrentWorld;
    ASSERT_NE(lastGood, view.m_routeEditBaseWorld);

    view.m_routeEditPreviewValid = false;
    view.m_routeEditInvalidPreviewWorld = lastGood;

    view.commitRouteEdit();

    const auto& out = dispatcher.document();
    const auto* link = out.tryLink(picked.link);
    ASSERT_NE(link, nullptr);

    ASSERT_TRUE(link->hasRouteOverride())
        << "Releasing on an invalid preview should still commit the last valid candidate.";
}