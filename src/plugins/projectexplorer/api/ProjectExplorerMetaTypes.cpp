// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/api/ProjectExplorerMetaTypes.hpp"

#include "projectexplorer/api/ProjectExplorerTypes.hpp"

#include <QtCore/QMetaType>

namespace ProjectExplorer {

void registerProjectExplorerMetaTypes()
{
    qRegisterMetaType<ProjectEntryKind>("ProjectExplorer::ProjectEntryKind");
    qRegisterMetaType<ProjectExplorerActionSection>("ProjectExplorer::ProjectExplorerActionSection");
    qRegisterMetaType<ProjectExplorerActionSpec>("ProjectExplorer::ProjectExplorerActionSpec");
    qRegisterMetaType<ProjectEntry>("ProjectExplorer::ProjectEntry");
    qRegisterMetaType<ProjectEntryList>("ProjectExplorer::ProjectEntryList");
    qRegisterMetaType<ProjectEntryList>("QVector<ProjectExplorer::ProjectEntry>");
}

} // namespace ProjectExplorer
