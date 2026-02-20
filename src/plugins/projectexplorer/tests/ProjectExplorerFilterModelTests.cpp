// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "projectexplorer/ProjectExplorerFilterModel.hpp"
#include "projectexplorer/ProjectExplorerModel.hpp"
#include "projectexplorer/api/ProjectExplorerTypes.hpp"

using ProjectExplorer::ProjectEntry;
using ProjectExplorer::ProjectEntryKind;
using ProjectExplorer::ProjectEntryList;
using ProjectExplorer::Internal::ProjectExplorerFilterModel;
using ProjectExplorer::Internal::ProjectExplorerModel;

TEST(ProjectExplorerFilterModelTests, FiltersByChildMatch)
{
    ProjectExplorerModel model;
    ProjectEntryList entries;
    entries.push_back({ QStringLiteral("docs/readme.md"), ProjectEntryKind::Asset });
    entries.push_back({ QStringLiteral("docs/guide.txt"), ProjectEntryKind::Asset });
    entries.push_back({ QStringLiteral("src/main.cpp"), ProjectEntryKind::Asset });
    model.setEntries(entries);

    ProjectExplorerFilterModel filter;
    filter.setSourceModel(&model);
    filter.setFilterText(QStringLiteral("read"));

    const QModelIndex root = filter.index(0, 0, QModelIndex());
    ASSERT_TRUE(root.isValid());

    EXPECT_EQ(filter.rowCount(root), 1);

    const QModelIndex docs = filter.index(0, 0, root);
    ASSERT_TRUE(docs.isValid());
    EXPECT_EQ(filter.data(docs, Qt::DisplayRole).toString(), QStringLiteral("docs"));
    EXPECT_EQ(filter.rowCount(docs), 1);

    const QModelIndex readme = filter.index(0, 0, docs);
    ASSERT_TRUE(readme.isValid());
    EXPECT_EQ(filter.data(readme, Qt::DisplayRole).toString(), QStringLiteral("readme.md"));
}
