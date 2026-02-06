#include "projectexplorer/ProjectExplorerPanel.hpp"

#include "projectexplorer/ProjectExplorerService.hpp"
#include "projectexplorer/ProjectExplorerModel.hpp"
#include "projectexplorer/ProjectExplorerTreeState.hpp"
#include "projectexplorer/ProjectExplorerActions.hpp"
#include "projectexplorer/views/ProjectExplorerItemDelegate.hpp"
#include "projectexplorer/search/ProjectExplorerSearchController.hpp"
#include "projectexplorer/search/ProjectExplorerSearchIndex.hpp"
#include "projectexplorer/state/ProjectExplorerPanelState.hpp"

#include <utils/contextmenu/ContextMenu.hpp>
#include <utils/ui/SidebarPanelFrame.hpp>

#include <QtCore/QItemSelectionModel>
#include <QtCore/QModelIndex>
#include <QtCore/QVector>
#include <QtGui/QIcon>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QTreeView>
#include <QtWidgets/QVBoxLayout>

namespace ProjectExplorer::Internal {

namespace {

using Utils::ContextMenuAction;
using ProjectExplorer::ProjectExplorerActionSection;

struct MenuEntry {
    ProjectExplorerActions::Action action;
    QString text;
    bool requiresItem = false;
    bool disallowRoot = false;
};

QString revealLabel()
{
#if defined(Q_OS_MAC)
    return QStringLiteral("Reveal in Finder");
#elif defined(Q_OS_WIN)
    return QStringLiteral("Reveal in Explorer");
#else
    return QStringLiteral("Reveal in File Manager");
#endif
}

const QVector<MenuEntry>& menuEntries()
{
    static const QVector<MenuEntry> entries = {
        { ProjectExplorerActions::Action::Open, QStringLiteral("Open"), true, true },
        { ProjectExplorerActions::Action::Rename, QStringLiteral("Rename"), true, true },
        { ProjectExplorerActions::Action::Delete, QStringLiteral("Delete"), true, true },
        { ProjectExplorerActions::Action::Duplicate, QStringLiteral("Duplicate"), true, true },
        { ProjectExplorerActions::Action::Reveal, revealLabel(), true, true }
    };
    return entries;
}

const QVector<MenuEntry>& createEntries()
{
    static const QVector<MenuEntry> entries = {
        { ProjectExplorerActions::Action::NewFolder, QStringLiteral("New Folder"), false, false },
        { ProjectExplorerActions::Action::NewDesign, QStringLiteral("New Design"), false, false },
        { ProjectExplorerActions::Action::ImportAsset, QStringLiteral("Import Asset"), false, false }
    };
    return entries;
}

const QString kOpenRootActionId = QStringLiteral("projectExplorer.openRoot");

ContextMenuAction makeAction(ProjectExplorerActions::Action action, const QString& text, bool enabled)
{
    ContextMenuAction item = ContextMenuAction::item(ProjectExplorerActions::id(action), text);
    item.enabled = enabled;
    return item;
}

ContextMenuAction makeCustomAction(const ProjectExplorer::ProjectExplorerActionSpec& spec, bool enabled)
{
    ContextMenuAction item = ContextMenuAction::item(spec.id, spec.text);
    item.enabled = enabled;
    return item;
}

} // namespace

ProjectExplorerPanel::ProjectExplorerPanel(ProjectExplorerService* service, QWidget* parent)
    : QWidget(parent)
    , m_service(service)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_frame = new Utils::SidebarPanelFrame(this);
    layout->addWidget(m_frame, 1);

    m_frame->addAction(kOpenRootActionId,
                       QIcon(QStringLiteral(":/ui/icons/svg/folder.svg")),
                       QStringLiteral("Open Folder"));
    connect(m_frame, &Utils::SidebarPanelFrame::actionTriggered,
            this, &ProjectExplorerPanel::handlePanelAction);

    m_tree = new QTreeView(this);
    m_tree->setObjectName(QStringLiteral("ProjectExplorerTree"));
    m_tree->setHeaderHidden(true);
    m_tree->setModel(service ? service->model() : nullptr);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setUniformRowHeights(true);
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(0, Qt::AscendingOrder);
    auto* delegate = new ProjectExplorerItemDelegate(m_tree);
    m_tree->setItemDelegate(delegate);
    m_frame->setContentWidget(m_tree);

    m_frame->setSearchPlaceholder(QStringLiteral("Search"));
    m_frame->setSearchEnabled(false);

    if (m_service) {
        m_frame->setTitle(QStringLiteral("Project"));
        m_frame->setSubtitle(QString());
        m_frame->setViewOptions({ QStringLiteral("Project"),
                                  QStringLiteral("Project Files"),
                                  QStringLiteral("Open Files"),
                                  QStringLiteral("All Changed Files"),
                                  QStringLiteral("Scratches and Consoles") });
    }

    connect(m_tree, &QTreeView::collapsed, this, &ProjectExplorerPanel::collapseDescendants);
    connect(m_tree, &QTreeView::doubleClicked, this, &ProjectExplorerPanel::handleActivate);
    connect(m_tree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ProjectExplorerPanel::handleSelectionChanged);
    connect(m_tree, &QTreeView::customContextMenuRequested,
            this, &ProjectExplorerPanel::showContextMenu);

    if (m_service) {
        connect(m_service, &ProjectExplorerService::selectPathRequested,
                this, &ProjectExplorerPanel::handleSelectPath);
    }

    m_contextMenu = new Utils::ContextMenu(this);
    connect(m_contextMenu, &Utils::ContextMenu::actionTriggered,
            this, &ProjectExplorerPanel::handleContextAction);

    m_treeState = new ProjectExplorerTreeState(m_service, this);
    m_treeState->attach(m_tree);
    if (m_service) {
        m_treeState->setRootPath(m_service->rootPath(), /*userInitiated=*/false);
        connect(m_service, &ProjectExplorerService::rootPathChanged,
                m_treeState, &ProjectExplorerTreeState::setRootPath);
    }

    m_panelState = new ProjectExplorerPanelState(m_service, this);
    m_panelState->attach(m_tree, m_frame);
    if (m_service) {
        m_panelState->setRootPath(m_service->rootPath());
        connect(m_service, &ProjectExplorerService::rootPathChanged,
                m_panelState, &ProjectExplorerPanelState::setRootPath);
    }

    m_searchIndex = new ProjectExplorerSearchIndex(this);
    if (m_service) {
        connect(m_service, &ProjectExplorerService::entriesChanged,
                m_searchIndex, &ProjectExplorerSearchIndex::setEntries);
    }

    m_searchController = new ProjectExplorerSearchController(m_tree, m_frame, m_treeState,
                                                             m_service, m_searchIndex, delegate, this);
    connect(m_searchIndex, &ProjectExplorerSearchIndex::indexRebuilt,
            m_searchController, &ProjectExplorerSearchController::refreshMatches);
}

void ProjectExplorerPanel::collapseDescendants(const QModelIndex& index)
{
    if (!index.isValid())
        return;

    const int rows = m_tree->model()->rowCount(index);
    for (int r = 0; r < rows; ++r) {
        const QModelIndex child = m_tree->model()->index(r, 0, index);
        if (child.isValid()) {
            m_tree->setExpanded(child, false);
            collapseDescendants(child);
        }
    }
}

void ProjectExplorerPanel::handleActivate(const QModelIndex& index)
{
    if (!m_service || !index.isValid())
        return;

    const int kind = index.data(ProjectExplorerModel::KindRole).toInt();
    const bool isFolder = (kind == static_cast<int>(ProjectExplorerModel::NodeKind::Root)
                           || kind == static_cast<int>(ProjectExplorerModel::NodeKind::Folder));
    if (isFolder) {
        const bool expanded = m_tree->isExpanded(index);
        m_tree->setExpanded(index, !expanded);
        return;
    }

    m_service->requestOpen(index);
}

void ProjectExplorerPanel::handleSelectionChanged(const QModelIndex& current, const QModelIndex&)
{
    if (m_service)
        m_service->requestSelectionChanged(current);
}

void ProjectExplorerPanel::showContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_tree->indexAt(pos);
    m_contextIndex = index;

    const bool hasItem = index.isValid();
    const int kind = hasItem ? index.data(ProjectExplorerModel::KindRole).toInt() : -1;
    const bool isRoot = (kind == static_cast<int>(ProjectExplorerModel::NodeKind::Root));

    QList<ContextMenuAction> actions;
    for (const auto& entry : menuEntries()) {
        const bool enabled = (!entry.requiresItem || hasItem) && (!entry.disallowRoot || !isRoot);
        actions.push_back(makeAction(entry.action, entry.text, enabled));
    }

    if (m_service) {
        const auto registered = m_service->registeredActions();
        for (const auto& spec : registered) {
            if (spec.section != ProjectExplorerActionSection::Primary)
                continue;
            const bool enabled = (!spec.requiresItem || hasItem) && (!spec.disallowRoot || !isRoot);
            actions.push_back(makeCustomAction(spec, enabled));
        }
    }

    actions.push_back(ContextMenuAction::separatorAction());
    for (const auto& entry : createEntries()) {
        actions.push_back(makeAction(entry.action, entry.text, true));
    }

    if (m_service) {
        const auto registered = m_service->registeredActions();
        for (const auto& spec : registered) {
            if (spec.section != ProjectExplorerActionSection::Create)
                continue;
            const bool enabled = (!spec.requiresItem || hasItem) && (!spec.disallowRoot || !isRoot);
            actions.push_back(makeCustomAction(spec, enabled));
        }

        bool hasCustom = false;
        for (const auto& spec : registered) {
            if (spec.section == ProjectExplorerActionSection::Custom) {
                hasCustom = true;
                break;
            }
        }
        if (hasCustom) {
            actions.push_back(ContextMenuAction::separatorAction());
            for (const auto& spec : registered) {
                if (spec.section != ProjectExplorerActionSection::Custom)
                    continue;
                const bool enabled = (!spec.requiresItem || hasItem) && (!spec.disallowRoot || !isRoot);
                actions.push_back(makeCustomAction(spec, enabled));
            }
        }
    }

    m_contextMenu->setActions(actions);
    m_contextMenu->exec(m_tree->viewport()->mapToGlobal(pos));
}

void ProjectExplorerPanel::handleContextAction(const QString& id)
{
    const bool hasIndex = m_contextIndex.isValid();
    const auto actionOpt = ProjectExplorerActions::fromId(id);
    const bool requiresIndex = actionOpt && (*actionOpt == ProjectExplorerActions::Action::Open
                                             || *actionOpt == ProjectExplorerActions::Action::Rename
                                             || *actionOpt == ProjectExplorerActions::Action::Delete
                                             || *actionOpt == ProjectExplorerActions::Action::Duplicate
                                             || *actionOpt == ProjectExplorerActions::Action::Reveal);
    if (requiresIndex && !hasIndex)
        return;

    if (actionOpt && *actionOpt == ProjectExplorerActions::Action::Rename && hasIndex) {
        m_tree->edit(m_contextIndex);
    }

    if (m_service) {
        const QModelIndex index = hasIndex ? QModelIndex(m_contextIndex) : QModelIndex();
        m_service->requestContextAction(id, index);
    }
}

void ProjectExplorerPanel::handlePanelAction(const QString& id)
{
    if (id == kOpenRootActionId && m_service)
        m_service->openRoot();
}

void ProjectExplorerPanel::handleSelectPath(const QString& path)
{
    if (!m_service || path.isEmpty())
        return;

    const QModelIndex index = m_service->indexForPath(path);
    if (!index.isValid())
        return;

    QModelIndex parent = index.parent();
    while (parent.isValid()) {
        m_tree->setExpanded(parent, true);
        parent = parent.parent();
    }

    m_tree->setCurrentIndex(index);
    m_tree->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

} // namespace ProjectExplorer::Internal
