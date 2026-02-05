#include "core/ui/UiHostImpl.hpp"

#include <QtWidgets/QLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QFrame>

#include <extensionsystem/PluginManager.hpp>

#include "core/api/ISidebarRegistry.hpp"
#include "core/api/SidebarToolSpec.hpp"

#include "core/widgets/FrameWidget.hpp"
#include "core/widgets/PlaygroundWidget.hpp"
#include "core/widgets/GlobalMenuBarWidget.hpp"
#include "core/widgets/CommandRibbonWidget.hpp"
#include "core/widgets/ToolRailWidget.hpp"
#include "core/widgets/SidebarOverlayHostWidget.hpp"
#include "core/widgets/InfoBarWidget.hpp"

#include "core/GlobalMenuBar.hpp"
#include "core/CommandRibbon.hpp"
#include "core/SidebarModel.hpp"
#include "core/SidebarRegistryImpl.hpp"

namespace Core {

static QWidget* ensureSingleSlotHost(QWidget* host)
{
    if (!host->layout()) {
        auto* l = new QVBoxLayout(host);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
    }
    return host;
}

UiHostImpl::UiHostImpl(FrameWidget* frame, QObject* parent)
    : IUiHost(parent)
    , m_frame(frame)
    , m_playground(frame ? frame->playground() : nullptr)
{
    m_menuModel = new GlobalMenuBar(this);
    m_ribbonModel = new CommandRibbon(this);
    m_sidebarRegistry = new SidebarRegistryImpl(this);
    ExtensionSystem::PluginManager::addObject(static_cast<ISidebarRegistry*>(m_sidebarRegistry));

    if (m_frame && m_frame->menuBarWidget())
        m_frame->menuBarWidget()->setModel(m_menuModel);

    if (m_frame && m_frame->ribbonHost()) {
        m_ribbonWidget = new CommandRibbonWidget;
        m_ribbonWidget->setModel(m_ribbonModel);
        replaceSingleChild(m_frame->ribbonHost(), m_ribbonWidget);
    }

    connect(m_menuModel, &GlobalMenuBar::activeChanged,
            this, &IUiHost::activeMenuTabChanged);
    connect(m_menuModel, &GlobalMenuBar::activeChanged, this, [this](const QString& id) {
        if (!m_ribbonModel)
            return;
        const RibbonResult r = m_ribbonModel->setActivePageId(id);
        (void)r;
    });

    if (m_playground && m_sidebarRegistry && m_sidebarRegistry->model()) {

                {
            auto* leftRailStack = new QWidget;
            leftRailStack->setObjectName("LeftSidebarRailStack");
            auto* leftRailLayout = new QVBoxLayout(leftRailStack);
            leftRailLayout->setContentsMargins(0, 0, 0, 0);
            leftRailLayout->setSpacing(0);

			auto* leftVerticalRail = new ToolRailWidget(m_sidebarRegistry->model(),
			                                           SidebarSide::Left,
			                                           SidebarFamily::Vertical,
			                                           leftRailStack);
			leftRailLayout->addWidget(leftVerticalRail, 0);

            auto* leftFamilySep = new QFrame(leftRailStack);
            leftFamilySep->setObjectName("ToolRailFamilySeparator");
            leftFamilySep->setFrameShape(QFrame::HLine);
            leftFamilySep->setFixedHeight(1);
            leftFamilySep->setAttribute(Qt::WA_StyledBackground, true);
			auto* leftHorizontalRail = new ToolRailWidget(m_sidebarRegistry->model(),
			                                             SidebarSide::Left,
			                                             SidebarFamily::Horizontal,
			                                             leftRailStack);
			leftRailLayout->addWidget(leftFamilySep, 0);
			leftRailLayout->addStretch(1);
			leftRailLayout->addWidget(leftHorizontalRail, 0);

			auto syncLeftFamilySep = [leftFamilySep, leftVerticalRail, leftHorizontalRail]() {
				leftFamilySep->setVisible(leftVerticalRail->isVisible() && leftHorizontalRail->isVisible());
			};
			syncLeftFamilySep();
			connect(m_sidebarRegistry->model(), &SidebarModel::railToolsChanged, leftRailStack,
			        [syncLeftFamilySep](SidebarSide, SidebarFamily, SidebarRail) { syncLeftFamilySep(); });
			connect(m_sidebarRegistry->model(), &SidebarModel::toolRegistered, leftRailStack,
			        [syncLeftFamilySep](const QString&) { syncLeftFamilySep(); });
			connect(m_sidebarRegistry->model(), &SidebarModel::toolUnregistered, leftRailStack,
			        [syncLeftFamilySep](const QString&) { syncLeftFamilySep(); });

            replaceSingleChild(m_playground->leftSidebarHost(), leftRailStack);

            auto* rightRailStack = new QWidget;
            rightRailStack->setObjectName("RightSidebarRailStack");
            auto* rightRailLayout = new QVBoxLayout(rightRailStack);
            rightRailLayout->setContentsMargins(0, 0, 0, 0);
            rightRailLayout->setSpacing(0);

			auto* rightVerticalRail = new ToolRailWidget(m_sidebarRegistry->model(),
			                                            SidebarSide::Right,
			                                            SidebarFamily::Vertical,
			                                            rightRailStack);
			rightRailLayout->addWidget(rightVerticalRail, 0);

            auto* rightFamilySep = new QFrame(rightRailStack);
            rightFamilySep->setObjectName("ToolRailFamilySeparator");
            rightFamilySep->setFrameShape(QFrame::HLine);
            rightFamilySep->setFixedHeight(1);
            rightFamilySep->setAttribute(Qt::WA_StyledBackground, true);
			auto* rightHorizontalRail = new ToolRailWidget(m_sidebarRegistry->model(),
			                                              SidebarSide::Right,
			                                              SidebarFamily::Horizontal,
			                                              rightRailStack);
			rightRailLayout->addWidget(rightFamilySep, 0);
			rightRailLayout->addStretch(1);
			rightRailLayout->addWidget(rightHorizontalRail, 0);

			auto syncRightFamilySep = [rightFamilySep, rightVerticalRail, rightHorizontalRail]() {
				rightFamilySep->setVisible(rightVerticalRail->isVisible() && rightHorizontalRail->isVisible());
			};
			syncRightFamilySep();
			connect(m_sidebarRegistry->model(), &SidebarModel::railToolsChanged, rightRailStack,
			        [syncRightFamilySep](SidebarSide, SidebarFamily, SidebarRail) { syncRightFamilySep(); });
			connect(m_sidebarRegistry->model(), &SidebarModel::toolRegistered, rightRailStack,
			        [syncRightFamilySep](const QString&) { syncRightFamilySep(); });
			connect(m_sidebarRegistry->model(), &SidebarModel::toolUnregistered, rightRailStack,
			        [syncRightFamilySep](const QString&) { syncRightFamilySep(); });

            replaceSingleChild(m_playground->rightSidebarHost(), rightRailStack);
        }

        if (auto* leftPanelHost = m_playground->leftSidebarPanelHost()) {
            auto* leftPanelStack = new QWidget(leftPanelHost);
            leftPanelStack->setObjectName("LeftSidebarPanelStack");
            auto* lay = new QVBoxLayout(leftPanelStack);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(0);

            lay->addWidget(new SidebarOverlayHostWidget(m_sidebarRegistry->model(),
                                                        SidebarSide::Left,
                                                        SidebarFamily::Vertical,
                                                        leftPanelStack),
                           1);

            lay->addWidget(new SidebarOverlayHostWidget(m_sidebarRegistry->model(),
                                                        SidebarSide::Left,
                                                        SidebarFamily::Horizontal,
                                                        leftPanelStack),
                           1);

            replaceSingleChild(leftPanelHost, leftPanelStack);
        }

        if (auto* rightPanelHost = m_playground->rightSidebarPanelHost()) {
            auto* rightPanelStack = new QWidget(rightPanelHost);
            rightPanelStack->setObjectName("RightSidebarPanelStack");
            auto* lay = new QVBoxLayout(rightPanelStack);
            lay->setContentsMargins(0, 0, 0, 0);
            lay->setSpacing(0);

            lay->addWidget(new SidebarOverlayHostWidget(m_sidebarRegistry->model(),
                                                        SidebarSide::Right,
                                                        SidebarFamily::Vertical,
                                                        rightPanelStack),
                           1);

            lay->addWidget(new SidebarOverlayHostWidget(m_sidebarRegistry->model(),
                                                        SidebarSide::Right,
                                                        SidebarFamily::Horizontal,
                                                        rightPanelStack),
                           1);

            replaceSingleChild(rightPanelHost, rightPanelStack);
        }
    }
}

bool UiHostImpl::addMenuTab(QString id, QString title)
{
    if (!m_ribbonModel)
        return false;

    const bool ok = m_menuModel->addItem(id, title);
    if (ok)
        ensureRibbonPage(std::move(id), std::move(title));
    return ok;
}

bool UiHostImpl::setActiveMenuTab(QString id)
{
    if (!m_menuModel)
        return false;
    return m_menuModel->setActiveId(id);
}

QString UiHostImpl::activeMenuTab() const
{
    return m_menuModel ? m_menuModel->activeId() : QString();
}

bool UiHostImpl::ensureRibbonPage(QString pageId, QString title)
{
    if (!m_ribbonModel)
        return false;
    return m_ribbonModel->ensurePage(pageId, title) != nullptr;
}

bool UiHostImpl::ensureRibbonGroup(QString pageId, QString groupId, QString title)
{
    if (!m_ribbonModel)
        return false;
    auto* page = m_ribbonModel->pageById(pageId);
    if (!page)
        return false;
    return page->ensureGroup(groupId, title) != nullptr;
}

RibbonResult UiHostImpl::setRibbonGroupLayout(QString pageId,
                                              QString groupId,
                                              std::unique_ptr<RibbonNode> root)
{
    if (!m_ribbonModel)
        return RibbonResult::failure("Ribbon: model not available.");
    auto* page = m_ribbonModel->pageById(pageId);
    if (!page)
        return RibbonResult::failure(QString("Ribbon: unknown page id '%1'.").arg(pageId));
    auto* group = page->groupById(groupId);
    if (!group)
        return RibbonResult::failure(QString("Ribbon: unknown group id '%1' on page '%2'.").arg(groupId, pageId));
    return group->setLayout(std::move(root));
}

RibbonResult UiHostImpl::addRibbonCommand(QString pageId,
                                         QString groupId,
                                         QString itemId,
                                         QAction* action,
                                         RibbonControlType type,
                                         RibbonPresentation pres)
{
    if (!m_ribbonModel)
        return RibbonResult::failure("Ribbon: model not available.");
    auto* page = m_ribbonModel->pageById(pageId);
    if (!page)
        return RibbonResult::failure(QString("Ribbon: unknown page id '%1'.").arg(pageId));
    auto* group = page->groupById(groupId);
    if (!group)
        return RibbonResult::failure(QString("Ribbon: unknown group id '%1' on page '%2'.").arg(groupId, pageId));
    return group->addAction(itemId, action, type, pres);
}

RibbonResult UiHostImpl::addRibbonSeparator(QString pageId, QString groupId, QString itemId)
{
    if (!m_ribbonModel)
        return RibbonResult::failure("Ribbon: model not available.");
    auto* page = m_ribbonModel->pageById(pageId);
    if (!page)
        return RibbonResult::failure(QString("Ribbon: unknown page id '%1'.").arg(pageId));
    auto* group = page->groupById(groupId);
    if (!group)
        return RibbonResult::failure(QString("Ribbon: unknown group id '%1' on page '%2'.").arg(groupId, pageId));
    return group->addSeparator(itemId);
}

RibbonResult UiHostImpl::addRibbonStretch(QString pageId, QString groupId, QString itemId)
{
    if (!m_ribbonModel)
        return RibbonResult::failure("Ribbon: model not available.");
    auto* page = m_ribbonModel->pageById(pageId);
    if (!page)
        return RibbonResult::failure(QString("Ribbon: unknown page id '%1'.").arg(pageId));
    auto* group = page->groupById(groupId);
    if (!group)
        return RibbonResult::failure(QString("Ribbon: unknown group id '%1' on page '%2'.").arg(groupId, pageId));
    return group->addStretch(itemId);
}

void UiHostImpl::replaceSingleChild(QWidget* host, QWidget* child)
{
    if (!host || !child)
        return;

    ensureSingleSlotHost(host);

    auto* l = host->layout();
    while (QLayoutItem* item = l->takeAt(0)) {
        if (QWidget* w = item->widget())
            w->deleteLater();
        delete item;
    }

    child->setParent(host);
    l->addWidget(child);
}

ISidebarRegistry* UiHostImpl::sidebarRegistry() const
{
    return m_sidebarRegistry;
}

void UiHostImpl::setLeftSidebar(QWidget* w)
{
    if (!m_playground) return;
    replaceSingleChild(m_playground->leftSidebarHost(), w);
}

void UiHostImpl::setRightSidebar(QWidget* w)
{
    if (!m_playground) return;
    replaceSingleChild(m_playground->rightSidebarHost(), w);
}

void UiHostImpl::setPlaygroundTopBar(QWidget* w)
{
    if (!m_playground) return;
    replaceSingleChild(m_playground->topBar(), w);
}

void UiHostImpl::setPlaygroundBottomBar(QWidget* w)
{
    if (!m_playground) return;
    replaceSingleChild(m_playground->bottomBar(), w);
}

InfoBarWidget* UiHostImpl::playgroundBottomBar() const
{
    return m_playground ? m_playground->bottomBar() : nullptr;
}

void UiHostImpl::setPlaygroundCenterBase(QWidget* w)
{
    if (!m_playground) return;
    replaceSingleChild(m_playground->centerBaseHost(), w);
}

QWidget* UiHostImpl::playgroundOverlayHost() const
{
    return m_playground ? m_playground->overlayHost() : nullptr;
}

} // namespace Core
