#include "projectexplorer/search/ProjectExplorerSearchMatcher.hpp"

#include <QtCore/Qt>

namespace ProjectExplorer::Internal {

ProjectExplorerMatchResult ProjectExplorerSearchMatcher::match(const QString& text,
                                                                const QString& query,
                                                                Qt::CaseSensitivity sensitivity)
{
    ProjectExplorerMatchResult result;
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return result;

    const int index = text.indexOf(trimmed, 0, sensitivity);
    if (index < 0)
        return result;

    result.matched = true;
    result.start = index;
    result.length = trimmed.size();
    return result;
}

} // namespace ProjectExplorer::Internal
