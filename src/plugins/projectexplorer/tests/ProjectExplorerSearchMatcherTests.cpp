// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/search/ProjectExplorerSearchMatcher.hpp"

#include <QtCore/QString>
#include <gtest/gtest.h>

using ProjectExplorer::Internal::ProjectExplorerSearchMatcher;

TEST(ProjectExplorerSearchMatcherTests, MatchesSubstringCaseInsensitive)
{
    const auto result = ProjectExplorerSearchMatcher::match(QStringLiteral("CMakeLists.txt"),
                                                            QStringLiteral("makel"));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.start, 1);
    EXPECT_EQ(result.length, 5);
}

TEST(ProjectExplorerSearchMatcherTests, NoMatchReturnsDefaults)
{
    const auto result = ProjectExplorerSearchMatcher::match(QStringLiteral("README.md"),
                                                            QStringLiteral("project"));
    EXPECT_FALSE(result.matched);
    EXPECT_EQ(result.start, -1);
    EXPECT_EQ(result.length, 0);
}
