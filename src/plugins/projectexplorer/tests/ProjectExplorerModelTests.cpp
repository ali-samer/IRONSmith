#include <gtest/gtest.h>

#include "projectexplorer/ProjectExplorerModel.hpp"
#include "projectexplorer/api/ProjectExplorerTypes.hpp"

using ProjectExplorer::ProjectEntry;
using ProjectExplorer::ProjectEntryKind;
using ProjectExplorer::ProjectEntryList;
using ProjectExplorer::Internal::ProjectExplorerModel;

TEST(ProjectExplorerModelTests, BuildsTreeAndIndexes)
{
    ProjectExplorerModel model;
    model.setRootLabel(QStringLiteral("MyProject"));

    ProjectEntryList entries;
    entries.push_back({ QStringLiteral("src/main.cpp"), ProjectEntryKind::Asset });
    entries.push_back({ QStringLiteral("docs/readme.md"), ProjectEntryKind::Asset });
    entries.push_back({ QStringLiteral("design.graphml"), ProjectEntryKind::Design });
    model.setEntries(entries);

    const QModelIndex root = model.index(0, 0, QModelIndex());
    ASSERT_TRUE(root.isValid());
    EXPECT_EQ(model.data(root, Qt::DisplayRole).toString(), QStringLiteral("MyProject"));
    EXPECT_EQ(model.data(root, ProjectExplorerModel::KindRole).toInt(),
              static_cast<int>(ProjectExplorerModel::NodeKind::Root));

    const QModelIndex srcFolder = model.indexForPath(QStringLiteral("src"));
    ASSERT_TRUE(srcFolder.isValid());
    EXPECT_TRUE(model.data(srcFolder, ProjectExplorerModel::IsFolderRole).toBool());

    const QModelIndex srcFile = model.indexForPath(QStringLiteral("src/main.cpp"));
    ASSERT_TRUE(srcFile.isValid());
    EXPECT_EQ(model.data(srcFile, ProjectExplorerModel::PathRole).toString(), QStringLiteral("src/main.cpp"));

    const QModelIndex design = model.indexForPath(QStringLiteral("design.graphml"));
    ASSERT_TRUE(design.isValid());
    EXPECT_EQ(model.data(design, ProjectExplorerModel::KindRole).toInt(),
              static_cast<int>(ProjectExplorerModel::NodeKind::Design));
}
