// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "utils/PathUtils.hpp"

using Utils::PathUtils::basename;
using Utils::PathUtils::ensureExtension;
using Utils::PathUtils::extension;
using Utils::PathUtils::hasExtension;
using Utils::PathUtils::normalizePath;
using Utils::PathUtils::sanitizeFileName;
using Utils::PathUtils::stem;

TEST(PathUtilsTests, NormalizeAndParts)
{
    EXPECT_EQ(normalizePath(u"foo/../bar//baz"), "bar/baz");
    EXPECT_EQ(basename(u"bar/baz.txt"), "baz.txt");
    EXPECT_EQ(stem(u"bar/baz.txt"), "baz");
    EXPECT_EQ(extension(u"bar/baz.txt"), "txt");
}

TEST(PathUtilsTests, ExtensionHelpers)
{
    EXPECT_TRUE(hasExtension(u"foo.json", u"json"));
    EXPECT_TRUE(hasExtension(u"foo.JSON", u"json"));
    EXPECT_EQ(ensureExtension(u"foo", u"json"), "foo.json");
    EXPECT_EQ(ensureExtension(u"foo.json", u".json"), "foo.json");
}

TEST(PathUtilsTests, SanitizeFileName)
{
    EXPECT_EQ(sanitizeFileName(u"report:2026/02/05"), "report_2026_02_05");
    EXPECT_EQ(sanitizeFileName(u"   "), "untitled");
}
