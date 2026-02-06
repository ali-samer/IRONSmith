#include "projectexplorer/ProjectExplorerActions.hpp"

#include <QtCore/QVector>

namespace ProjectExplorer::Internal {

namespace {

struct ActionEntry {
    ProjectExplorerActions::Action action;
    QString id;
};

const QVector<ActionEntry>& actionEntries()
{
    static const QVector<ActionEntry> entries = {
        { ProjectExplorerActions::Action::Open, QStringLiteral("open") },
        { ProjectExplorerActions::Action::Rename, QStringLiteral("rename") },
        { ProjectExplorerActions::Action::Delete, QStringLiteral("delete") },
        { ProjectExplorerActions::Action::Duplicate, QStringLiteral("duplicate") },
        { ProjectExplorerActions::Action::Reveal, QStringLiteral("reveal") },
        { ProjectExplorerActions::Action::NewFolder, QStringLiteral("new-folder") },
        { ProjectExplorerActions::Action::NewDesign, QStringLiteral("new-design") },
        { ProjectExplorerActions::Action::ImportAsset, QStringLiteral("import-asset") }
    };
    return entries;
}

} // namespace

QString ProjectExplorerActions::id(Action action)
{
    for (const auto& entry : actionEntries()) {
        if (entry.action == action)
            return entry.id;
    }
    return {};
}

std::optional<ProjectExplorerActions::Action> ProjectExplorerActions::fromId(QStringView id)
{
    if (id.isEmpty())
        return std::nullopt;

    for (const auto& entry : actionEntries()) {
        if (entry.id == id)
            return entry.action;
    }
    return std::nullopt;
}

} // namespace ProjectExplorer::Internal
