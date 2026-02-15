// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "core/widgets/SidebarOverlayResizeGrip.hpp"

#include <QtGui/QMouseEvent>

#include "core/widgets/SidebarOverlayHostWidget.hpp"

namespace Core {

SidebarOverlayResizeGrip::SidebarOverlayResizeGrip(SidebarOverlayHostWidget* owner,
                                                   SidebarSide side,
                                                   QWidget* parent)
    : QWidget(parent)
    , m_owner(owner)
    , m_side(side)
{
    setObjectName("SidebarOverlayResizeGrip");
    setFixedWidth(kGripPx);
    setCursor(Qt::SplitHCursor);
    setMouseTracking(true);
    setAttribute(Qt::WA_StyledBackground, false);
}

void SidebarOverlayResizeGrip::mousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton || !m_owner || !m_owner->hasPanels()) {
        QWidget::mousePressEvent(e);
        return;
    }

    m_resizing = true;
    m_pressGlobalX = e->globalPosition().x();
    m_pressPanelW = m_owner->panelWidth();
    grabMouse();
    e->accept();
}

void SidebarOverlayResizeGrip::mouseMoveEvent(QMouseEvent* e)
{
    if (!m_resizing || !m_owner) {
        QWidget::mouseMoveEvent(e);
        return;
    }

    const int dx = int(e->globalPosition().x() - m_pressGlobalX);

    int newW = m_pressPanelW;
    if (m_side == SidebarSide::Left)
        newW += dx;
    else
        newW -= dx;

    m_owner->setPanelWidthClamped(newW);
    e->accept();
}

void SidebarOverlayResizeGrip::mouseReleaseEvent(QMouseEvent* e)
{
    if (m_resizing && e->button() == Qt::LeftButton) {
        m_resizing = false;
        releaseMouse();
        e->accept();
        return;
    }
    QWidget::mouseReleaseEvent(e);
}

} // namespace Core
