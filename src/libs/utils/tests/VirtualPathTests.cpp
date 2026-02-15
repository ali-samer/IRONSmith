// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "utils/VirtualPath.hpp"

#include <QtCore/QDir>

using Utils::VirtualPath;

TEST(VirtualPathTests, FileSystemNormalization)
{
    VirtualPath p = VirtualPath::fromFileSystem("foo/../bar//baz.txt");
    EXPECT_EQ(p.toString(), "bar/baz.txt");
    EXPECT_TRUE(p.isRelative());
    EXPECT_EQ(p.basename(), "baz.txt");
    EXPECT_EQ(p.stem(), "baz");
    EXPECT_EQ(p.extension(), "txt");
}

TEST(VirtualPathTests, FileSystemAbsoluteDetection)
{
    const QString root = QDir::rootPath();
    VirtualPath p = VirtualPath::fromFileSystem(root + "var/log");
    EXPECT_TRUE(p.isAbsolute());
    EXPECT_TRUE(p.toString().contains("var/log"));
}

TEST(VirtualPathTests, BundleJoinAndParent)
{
    VirtualPath p = VirtualPath::fromBundle("design/graph.json");
    VirtualPath joined = p.parent().join(u"meta/settings.json");

    EXPECT_EQ(p.parent().toString(), "design");
    EXPECT_EQ(joined.toString(), "design/meta/settings.json");
    EXPECT_TRUE(joined.startsWith(VirtualPath::fromBundle("design")));
}
