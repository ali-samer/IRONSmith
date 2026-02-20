// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "core/widgets/FrameWidget.hpp"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "core/widgets/GlobalMenuBarWidget.hpp"
#include "core/widgets/PlaygroundWidget.hpp"
#include "core/ui/UiStyle.hpp"
#include "core/ui/UiObjectNames.hpp"

namespace Core {

FrameWidget::FrameWidget(QWidget* parent)
	: QWidget(parent)
{
	setObjectName(UiObjectNames::FrameRoot);
	setAttribute(Qt::WA_StyledBackground, true);

	auto* root = new QVBoxLayout(this);
	root->setContentsMargins(0, 0, 0, 0);
	root->setSpacing(0);

	m_menuBarWidget = new GlobalMenuBarWidget(this);
	m_menuBarWidget->setFixedHeight(Ui::UiStyle::MenuBarHeightPx);
	root->addWidget(m_menuBarWidget);

	m_ribbonHost = new QWidget(this);
	m_ribbonHost->setObjectName(UiObjectNames::RibbonHost);
	m_ribbonHost->setAttribute(Qt::WA_StyledBackground, true);
	m_ribbonHost->setFixedHeight(Ui::UiStyle::RibbonHostHeightPx);
	root->addWidget(m_ribbonHost);

	m_playground = new PlaygroundWidget(this);
	root->addWidget(m_playground, 1);
}

GlobalMenuBarWidget* FrameWidget::menuBarWidget() const
{
	return m_menuBarWidget;
}

QWidget* FrameWidget::ribbonHost() const
{
	return m_ribbonHost;
}

PlaygroundWidget* FrameWidget::playground() const
{
	return m_playground;
}

} // namespace Core
