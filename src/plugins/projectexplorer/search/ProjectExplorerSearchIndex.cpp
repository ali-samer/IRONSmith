#include "projectexplorer/search/ProjectExplorerSearchIndex.hpp"

#include <utils/async/AsyncTask.hpp>

#include <QtCore/QFileInfo>

#include <utility>

namespace ProjectExplorer::Internal {

namespace {

struct IndexPayload final {
    QVector<ProjectExplorerSearchIndex::Entry> entries;
};

IndexPayload buildIndex(const ProjectExplorer::ProjectEntryList& entries)
{
    IndexPayload payload;
    payload.entries.reserve(entries.size());

    for (const auto& entry : entries) {
        if (entry.path.isEmpty())
            continue;

        const QString name = QFileInfo(entry.path).fileName();
        if (name.isEmpty())
            continue;

        ProjectExplorerSearchIndex::Entry item;
        item.nameLower = name.toLower();
        item.path = entry.path;
        payload.entries.push_back(std::move(item));
    }

    return payload;
}

} // namespace

ProjectExplorerSearchIndex::ProjectExplorerSearchIndex(QObject* parent)
    : QObject(parent)
{
}

void ProjectExplorerSearchIndex::setEntries(const ProjectExplorer::ProjectEntryList& entries)
{
    const quint64 token = ++m_generation;
    m_ready = false;
    const ProjectExplorer::ProjectEntryList snapshot = entries;

    Utils::Async::run<IndexPayload>(this,
                                    [snapshot]() { return buildIndex(snapshot); },
                                    [this, token](IndexPayload payload) {
                                        if (token != m_generation.load())
                                            return;
                                        m_entries = std::move(payload.entries);
                                        m_ready = true;
                                        emit indexRebuilt();
                                    });
}

QVector<QString> ProjectExplorerSearchIndex::findMatches(const QString& query) const
{
    QVector<QString> matches;
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return matches;

    const QString needle = trimmed.toLower();
    for (const auto& entry : m_entries) {
        if (entry.nameLower.contains(needle))
            matches.push_back(entry.path);
    }

    return matches;
}

bool ProjectExplorerSearchIndex::isReady() const
{
    return m_ready;
}

} // namespace ProjectExplorer::Internal
