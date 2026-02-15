// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QMetaType>
#include <QtCore/QVector>
#include <QtCore/QString>

namespace ProjectExplorer {

enum class ProjectEntryKind : unsigned char {
    Folder,
    Design,
    Asset,
    Meta,
    Cache,
    Unknown
};

enum class ProjectExplorerActionSection : unsigned char {
    Primary,
    Create,
    Custom
};

struct PROJECTEXPLORER_EXPORT ProjectExplorerActionSpec final {
    QString id;
    QString text;
    ProjectExplorerActionSection section = ProjectExplorerActionSection::Custom;
    bool requiresItem = false;
    bool disallowRoot = false;
};

struct PROJECTEXPLORER_EXPORT ProjectEntry final {
    QString path;
    ProjectEntryKind kind = ProjectEntryKind::Unknown;
};

using ProjectEntryList = QVector<ProjectEntry>;
using ProjectExplorerActionList = QVector<ProjectExplorerActionSpec>;

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::ProjectEntryKind)
Q_DECLARE_METATYPE(ProjectExplorer::ProjectExplorerActionSection)
Q_DECLARE_METATYPE(ProjectExplorer::ProjectExplorerActionSpec)
Q_DECLARE_METATYPE(ProjectExplorer::ProjectEntry)
Q_DECLARE_METATYPE(ProjectExplorer::ProjectEntryList)
