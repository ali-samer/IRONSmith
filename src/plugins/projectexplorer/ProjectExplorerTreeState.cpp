#include "projectexplorer/ProjectExplorerTreeState.hpp"

#include "projectexplorer/ProjectExplorerService.hpp"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QModelIndex>
#include <QtCore/Qt>
#include <QtWidgets/QTreeView>

#include <utility>

namespace ProjectExplorer::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kTreeStateName = u"projectExplorer/treeState"_s;

} // namespace

ProjectExplorerTreeState::ProjectExplorerTreeState(ProjectExplorerService* service, QObject* parent)
    : QObject(parent)
    , m_env(makeEnvironment())
    , m_service(service)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProjectExplorerTreeState::flushSave);
}

ProjectExplorerTreeState::ProjectExplorerTreeState(ProjectExplorerService* service,
                                                   Utils::Environment environment,
                                                   QObject* parent)
    : QObject(parent)
    , m_env(std::move(environment))
    , m_service(service)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(250);
    connect(&m_saveTimer, &QTimer::timeout, this, &ProjectExplorerTreeState::flushSave);
}

void ProjectExplorerTreeState::attach(QTreeView* view)
{
    if (m_view == view)
        return;

    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
        if (m_view->model())
            disconnect(m_view->model(), nullptr, this, nullptr);
    }

    m_view = view;
    if (!m_view)
        return;

    connect(m_view, &QTreeView::expanded, this, &ProjectExplorerTreeState::handleExpanded);
    connect(m_view, &QTreeView::collapsed, this, &ProjectExplorerTreeState::handleCollapsed);

    if (auto* model = m_view->model()) {
        connect(model, &QAbstractItemModel::modelReset, this, &ProjectExplorerTreeState::handleModelReset);
        connect(model, &QAbstractItemModel::layoutChanged, this, &ProjectExplorerTreeState::handleModelReset);
    }
}

void ProjectExplorerTreeState::setRootPath(const QString& rootPath, bool userInitiated)
{
    const QString cleaned = rootPath.trimmed();
    if (cleaned == m_rootPath)
        return;

    m_rootPath = cleaned;
    loadStateForRoot(m_rootPath);

    if (userInitiated) {
        m_rootExpanded = true;
        scheduleSave();
    }

    apply();
}

void ProjectExplorerTreeState::setSuspended(bool suspended)
{
    m_suspended = suspended;
}

bool ProjectExplorerTreeState::isSuspended() const
{
    return m_suspended;
}

void ProjectExplorerTreeState::handleExpanded(const QModelIndex& index)
{
    if (m_applying)
        return;
    if (m_suspended)
        return;

    const QString path = pathForIndex(index);
    if (path.isEmpty()) {
        m_rootExpanded = true;
    } else {
        m_expanded.insert(path);
    }

    scheduleSave();
}

void ProjectExplorerTreeState::handleCollapsed(const QModelIndex& index)
{
    if (m_applying)
        return;
    if (m_suspended)
        return;

    const QString path = pathForIndex(index);
    if (path.isEmpty()) {
        m_rootExpanded = false;
    } else {
        m_expanded.remove(path);
    }

    scheduleSave();
}

void ProjectExplorerTreeState::handleModelReset()
{
    if (m_suspended)
        return;
    apply();
}

void ProjectExplorerTreeState::flushSave()
{
    saveState();
}

Utils::Environment ProjectExplorerTreeState::makeEnvironment()
{
    Utils::EnvironmentConfig cfg;
    cfg.organizationName = QStringLiteral("IRONSmith");
    cfg.applicationName = QStringLiteral("IRONSmith");
    return Utils::Environment(cfg);
}

void ProjectExplorerTreeState::loadStateForRoot(const QString& rootPath)
{
    m_expanded.clear();
    m_rootExpanded = true;

    if (rootPath.isEmpty())
        return;

    const auto loaded = m_env.loadState(Utils::EnvironmentScope::Global, kTreeStateName);
    if (loaded.status != Utils::DocumentLoadResult::Status::Ok)
        return;

    const QJsonObject roots = loaded.object.value(QStringLiteral("roots")).toObject();
    const QJsonObject rootState = roots.value(rootPath).toObject();

    if (!rootState.isEmpty()) {
        m_rootExpanded = rootState.value(QStringLiteral("rootExpanded")).toBool(true);
        const QJsonArray expanded = rootState.value(QStringLiteral("expanded")).toArray();
        for (const auto& value : expanded) {
            const QString path = value.toString();
            if (!path.isEmpty())
                m_expanded.insert(path);
        }
    }
}

void ProjectExplorerTreeState::apply()
{
    if (!m_view || !m_view->model())
        return;

    m_applying = true;

    const QModelIndex rootIndex = m_view->model()->index(0, 0);
    if (rootIndex.isValid())
        m_view->setExpanded(rootIndex, m_rootExpanded);

    if (m_service) {
        for (const QString& path : m_expanded) {
            const QModelIndex idx = m_service->indexForPath(path);
            if (idx.isValid())
                m_view->setExpanded(idx, true);
        }
    }

    m_applying = false;
}

void ProjectExplorerTreeState::scheduleSave()
{
    if (m_suspended)
        return;
    if (!m_saveTimer.isActive())
        m_saveTimer.start();
}

void ProjectExplorerTreeState::saveState()
{
    if (m_rootPath.isEmpty())
        return;

    QJsonObject doc;
    const auto loaded = m_env.loadState(Utils::EnvironmentScope::Global, kTreeStateName);
    if (loaded.status == Utils::DocumentLoadResult::Status::Ok)
        doc = loaded.object;

    QJsonObject roots = doc.value(QStringLiteral("roots")).toObject();

    QJsonObject rootState;
    rootState.insert(QStringLiteral("rootExpanded"), m_rootExpanded);

    QJsonArray expanded;
    for (const QString& path : m_expanded)
        expanded.append(path);
    rootState.insert(QStringLiteral("expanded"), expanded);

    roots.insert(m_rootPath, rootState);
    doc.insert(QStringLiteral("roots"), roots);

    m_env.saveState(Utils::EnvironmentScope::Global, kTreeStateName, doc);
}

QString ProjectExplorerTreeState::pathForIndex(const QModelIndex& index) const
{
    if (!m_service || !index.isValid())
        return {};
    return m_service->pathForIndex(index);
}

} // namespace ProjectExplorer::Internal
