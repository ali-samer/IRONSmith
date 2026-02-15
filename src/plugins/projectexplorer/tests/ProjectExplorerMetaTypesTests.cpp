// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "projectexplorer/api/ProjectExplorerMetaTypes.hpp"

#include <QtCore/QMetaType>

TEST(ProjectExplorerMetaTypesTests, RegistersMetaTypes)
{
    ProjectExplorer::registerProjectExplorerMetaTypes();

    EXPECT_NE(QMetaType::type("ProjectExplorer::ProjectEntryKind"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("ProjectExplorer::ProjectExplorerActionSection"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("ProjectExplorer::ProjectExplorerActionSpec"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("ProjectExplorer::ProjectEntry"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("ProjectExplorer::ProjectEntryList"), QMetaType::UnknownType);
    EXPECT_NE(QMetaType::type("QVector<ProjectExplorer::ProjectEntry>"), QMetaType::UnknownType);
}
