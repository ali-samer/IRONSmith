// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "utils/filesystem/PathPatternMatcher.hpp"

#include <gtest/gtest.h>

using Utils::PathPatternMatcher;

TEST(PathPatternMatcherTests, MatchesBasenamePatterns)
{
    PathPatternMatcher matcher;
    matcher.setPatterns({QStringLiteral("*.log"), QStringLiteral("build")});

    EXPECT_TRUE(matcher.matches(QStringLiteral("build"), true));
    EXPECT_TRUE(matcher.matches(QStringLiteral("other/build"), false));
    EXPECT_TRUE(matcher.matches(QStringLiteral("logs/app.log"), false));
    EXPECT_FALSE(matcher.matches(QStringLiteral("build/output.txt"), false));
    EXPECT_FALSE(matcher.matches(QStringLiteral("src/main.cpp"), false));
}

TEST(PathPatternMatcherTests, MatchesPathScopedPatterns)
{
    PathPatternMatcher matcher;
    matcher.setPatterns({QStringLiteral("cmake-build-*/**")});

    EXPECT_TRUE(matcher.matches(QStringLiteral("cmake-build-debug/CMakeCache.txt"), false));
    EXPECT_TRUE(matcher.matches(QStringLiteral("cmake-build-debug/subdir"), true));
    EXPECT_FALSE(matcher.matches(QStringLiteral("src/cmake-build-debug.txt"), false));
    EXPECT_FALSE(matcher.matches(QStringLiteral("cmake-build-release"), true));
}

TEST(PathPatternMatcherTests, DirectoryOnlyPatterns)
{
    PathPatternMatcher matcher;
    matcher.setPatterns({QStringLiteral("out/")});

    EXPECT_TRUE(matcher.matches(QStringLiteral("out"), true));
    EXPECT_FALSE(matcher.matches(QStringLiteral("out/file.txt"), false));
}
