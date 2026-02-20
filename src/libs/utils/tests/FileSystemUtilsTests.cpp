// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "utils/filesystem/FileSystemUtils.hpp"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QTemporaryDir>

using Utils::FileSystemUtils::duplicateName;
using Utils::FileSystemUtils::uniqueChildName;

TEST(FileSystemUtilsTests, UniqueChildNameIncrements)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());

    QDir dir(temp.path());
    const QString first = uniqueChildName(dir, QStringLiteral("Design"), QStringLiteral("ironsmith"));
    ASSERT_EQ(first, QStringLiteral("Design.ironsmith"));

    QFile existing(dir.filePath(first));
    ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
    existing.close();

    const QString second = uniqueChildName(dir, QStringLiteral("Design"), QStringLiteral("ironsmith"));
    EXPECT_EQ(second, QStringLiteral("Design (1).ironsmith"));
}

TEST(FileSystemUtilsTests, DuplicateNameUsesCopySuffix)
{
    QTemporaryDir temp;
    ASSERT_TRUE(temp.isValid());

    QDir dir(temp.path());
    const QString name = QStringLiteral("MyDesign.ironsmith");
    QFile existing(dir.filePath(name));
    ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
    existing.close();

    const QString copy = duplicateName(dir, name);
    EXPECT_EQ(copy, QStringLiteral("MyDesign copy.ironsmith"));
}
