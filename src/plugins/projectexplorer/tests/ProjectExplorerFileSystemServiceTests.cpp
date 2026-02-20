// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "projectexplorer/filesystem/ProjectExplorerFileSystemService.hpp"

#include <utils/DocumentBundle.hpp>

#include <QtCore/QDir>
#include <QtCore/QTemporaryDir>

using ProjectExplorer::Internal::ProjectExplorerFileSystemService;

TEST(ProjectExplorerFileSystemServiceTests, CreateDesignCreatesBundle)
{
    QTemporaryDir root;
    ASSERT_TRUE(root.isValid());

    ProjectExplorerFileSystemService fs;
    fs.setRootPath(root.path());

    QString relPath;
    const Utils::Result result = fs.createDesign(QString(), QStringLiteral("MyDesign"), &relPath);
    ASSERT_TRUE(result.ok) << result.errors.join("\n").toStdString();
    ASSERT_FALSE(relPath.isEmpty());

    const QString absPath = QDir(root.path()).filePath(relPath);
    QString error;
    EXPECT_TRUE(Utils::DocumentBundle::isBundle(absPath, &error)) << error.toStdString();
}
