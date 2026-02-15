// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "projectexplorer/ProjectExplorerGlobal.hpp"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringView>

#include <optional>

namespace ProjectExplorer::Internal {

class ProjectExplorerActions final
{
    Q_GADGET

public:
    enum class Action {
        Open,
        Rename,
        Delete,
        Duplicate,
        Reveal,
        NewFolder,
        NewDesign,
        ImportAsset
    };
    Q_ENUM(Action)

    static QString id(Action action);
    static std::optional<Action> fromId(QStringView id);
};

} // namespace ProjectExplorer::Internal
