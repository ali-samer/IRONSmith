#include "projectexplorer/state/ProjectExplorerPanelState.hpp"

#include "projectexplorer/ProjectExplorerService.hpp"

#include <utils/ui/SidebarPanelFrame.hpp>

#include <QtCore/QAbstractItemModel>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QModelIndex>
#include <QtCore/QTimer>
#include <QtWidgets/QTreeView>

namespace ProjectExplorer::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kPanelStateName = u"projectExplorer/panelState"_s;
const QString kViewIdKey = u"viewId"_s;
const QString kRootsKey = u"roots"_s;
const QString kSelectionKey = u"selection"_s;

} // namespace

ProjectExplorerPanelState::ProjectExplorerPanelState(ProjectExplorerService* service, QObject* parent)
    : QObject(parent)
    , m_env(makeEnvironment())
    , m_service(service)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProjectExplorerPanelState::flushSave);
}

ProjectExplorerPanelState::ProjectExplorerPanelState(ProjectExplorerService* service,
                                                     Utils::Environment environment,
                                                     QObject* parent)
    : QObject(parent)
    , m_env(std::move(environment))
    , m_service(service)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProjectExplorerPanelState::flushSave);
}

void ProjectExplorerPanelState::attach(QTreeView* view, Utils::SidebarPanelFrame* frame)
{
    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
        if (m_view->model())
            disconnect(m_view->model(), nullptr, this, nullptr);
    }

    m_view = view;
    m_frame = frame;

    if (m_view) {
        if (auto* model = m_view->model()) {
            connect(model, &QAbstractItemModel::modelReset, this, &ProjectExplorerPanelState::handleModelReset);
            connect(model, &QAbstractItemModel::layoutChanged, this, &ProjectExplorerPanelState::handleModelReset);
        }

        if (auto* selection = m_view->selectionModel()) {
            connect(selection, &QItemSelectionModel::currentChanged,
                    this, &ProjectExplorerPanelState::handleSelectionChanged);
        }
    }

    if (m_frame) {
        connect(m_frame, &Utils::SidebarPanelFrame::viewSelected,
                this, &ProjectExplorerPanelState::handleViewSelected);
    }

    applyView();
    applySelection();
}

void ProjectExplorerPanelState::setRootPath(const QString& rootPath)
{
    const QString cleaned = rootPath.trimmed();
    if (cleaned == m_rootPath)
        return;

    m_rootPath = cleaned;
    loadStateForRoot(m_rootPath);
    applySelection();
    applyView();
}

void ProjectExplorerPanelState::handleSelectionChanged(const QModelIndex& current, const QModelIndex&)
{
    if (m_applying || !m_service)
        return;

    const QString path = m_service->pathForIndex(current);
    if (path.isEmpty())
        return;

    if (path == m_selectedPath)
        return;

    m_selectedPath = path;
    scheduleSave();
}

void ProjectExplorerPanelState::handleViewSelected(const QString& viewId)
{
    if (m_applying)
        return;

    const QString cleaned = viewId.trimmed();
    if (cleaned.isEmpty() || cleaned == m_viewId)
        return;

    m_viewId = cleaned;
    scheduleSave();
}

void ProjectExplorerPanelState::handleModelReset()
{
    applySelection();
}

void ProjectExplorerPanelState::flushSave()
{
    saveState();
}

Utils::Environment ProjectExplorerPanelState::makeEnvironment()
{
    Utils::EnvironmentConfig cfg;
    cfg.organizationName = QStringLiteral("IRONSmith");
    cfg.applicationName = QStringLiteral("IRONSmith");
    return Utils::Environment(cfg);
}

void ProjectExplorerPanelState::loadStateForRoot(const QString& rootPath)
{
    m_selectedPath.clear();
    m_pendingSelection.clear();

    const auto loaded = m_env.loadState(Utils::EnvironmentScope::Global, kPanelStateName);
    if (loaded.status != Utils::DocumentLoadResult::Status::Ok)
        return;

    const QJsonObject doc = loaded.object;
    m_viewId = doc.value(kViewIdKey).toString();

    if (rootPath.isEmpty())
        return;

    const QJsonObject roots = doc.value(kRootsKey).toObject();
    const QJsonObject rootState = roots.value(rootPath).toObject();
    const QString selection = rootState.value(kSelectionKey).toString();

    if (!selection.isEmpty())
        m_pendingSelection = selection;
}

void ProjectExplorerPanelState::applySelection()
{
    if (!m_service || !m_view)
        return;

    if (m_pendingSelection.isEmpty())
        return;

    const QModelIndex idx = m_service->indexForPath(m_pendingSelection);
    if (!idx.isValid())
        return;

    m_applying = true;
    m_service->selectPath(m_pendingSelection);
    m_selectedPath = m_pendingSelection;
    m_pendingSelection.clear();
    QTimer::singleShot(0, this, [this]() { m_applying = false; });
}

void ProjectExplorerPanelState::applyView()
{
    if (!m_frame)
        return;

    if (m_viewId.isEmpty())
        return;

    const QStringList options = m_frame->viewOptions();
    if (!options.contains(m_viewId))
        return;

    m_applying = true;
    m_frame->setTitle(m_viewId);
    QTimer::singleShot(0, this, [this]() { m_applying = false; });
}

void ProjectExplorerPanelState::scheduleSave()
{
    if (!m_saveTimer.isActive())
        m_saveTimer.start();
}

void ProjectExplorerPanelState::saveState()
{
    QJsonObject doc;
    const auto loaded = m_env.loadState(Utils::EnvironmentScope::Global, kPanelStateName);
    if (loaded.status == Utils::DocumentLoadResult::Status::Ok)
        doc = loaded.object;

    doc.insert(kViewIdKey, m_viewId);

    if (!m_rootPath.isEmpty() && !m_selectedPath.isEmpty()) {
        QJsonObject roots = doc.value(kRootsKey).toObject();
        QJsonObject rootState = roots.value(m_rootPath).toObject();
        rootState.insert(kSelectionKey, m_selectedPath);
        roots.insert(m_rootPath, rootState);
        doc.insert(kRootsKey, roots);
    }

    m_env.saveState(Utils::EnvironmentScope::Global, kPanelStateName, doc);
}

} // namespace ProjectExplorer::Internal
