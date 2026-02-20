// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtWidgets/QWidget>

#include "core/api/SidebarToolSpec.hpp"

class QMouseEvent;

namespace Core {

class SidebarOverlayHostWidget;

class SidebarOverlayResizeGrip final : public QWidget
{
    Q_OBJECT

public:
    explicit SidebarOverlayResizeGrip(SidebarOverlayHostWidget* owner,
                                      SidebarSide side,
                                      QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;

private:
    static constexpr int kGripPx = 6;

    SidebarOverlayHostWidget* m_owner = nullptr; // non-owning
    SidebarSide m_side;
    bool m_resizing = false;
    qreal m_pressGlobalX = 0.0;
    int m_pressPanelW = 0;
};

} // namespace Core
