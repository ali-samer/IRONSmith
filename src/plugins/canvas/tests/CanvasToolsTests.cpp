// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "canvas/Tools.hpp"

namespace {

void expectPointNear(const QPointF& a, const QPointF& b, double eps = 1e-9)
{
    EXPECT_NEAR(a.x(), b.x(), eps);
    EXPECT_NEAR(a.y(), b.y(), eps);
}

} // namespace

TEST(CanvasToolsMathTests, RoundTripSceneView)
{
    struct Case { QPointF scene; QPointF pan; double zoom; };
    const std::vector<Case> cases = {
        {QPointF(0.0, 0.0), QPointF(0.0, 0.0), 1.0},
        {QPointF(12.5, -7.25), QPointF(3.0, 4.0), 1.0},
        {QPointF(-123.0, 456.0), QPointF(10.0, -20.0), 2.0},
        {QPointF(1.0, 2.0), QPointF(-3.0, -4.0), 0.5},
        {QPointF(999.125, -1001.75), QPointF(0.25, -0.75), 3.75},
    };

    for (const auto& c : cases) {
        const QPointF view = Canvas::Tools::sceneToView(c.scene, c.pan, c.zoom);
        const QPointF back = Canvas::Tools::viewToScene(view, c.pan, c.zoom);
        expectPointNear(back, c.scene);
    }
}

TEST(CanvasToolsMathTests, PanFromViewDragMatchesSceneDelta)
{
    const QPointF startPan(10.0, -5.0);
    const QPointF startView(100.0, 200.0);
    const QPointF currView(140.0, 170.0);
    const double zoom = 2.0;

    const QPointF pan = Canvas::Tools::panFromViewDrag(startPan, startView, currView, zoom);
    const QPointF deltaView = currView - startView;
    const QPointF expected = startPan + (deltaView / zoom);
    expectPointNear(pan, expected);
}
