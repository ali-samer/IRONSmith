// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include "projectexplorer/search/ProjectExplorerSearchIndex.hpp"

#include <QtCore/QCoreApplication>
#include <QtTest/QSignalSpy>
#include <gtest/gtest.h>
#include <memory>

using ProjectExplorer::Internal::ProjectExplorerSearchIndex;

namespace {

ProjectExplorer::ProjectEntryList makeEntries()
{
    ProjectExplorer::ProjectEntryList entries;
    entries.push_back({QStringLiteral("src/main.cpp"), ProjectExplorer::ProjectEntryKind::Asset});
    entries.push_back({QStringLiteral("docs/README.md"), ProjectExplorer::ProjectEntryKind::Asset});
    entries.push_back({QStringLiteral("cmake/CMakeLists.txt"), ProjectExplorer::ProjectEntryKind::Asset});
    return entries;
}

} // namespace

TEST(ProjectExplorerSearchIndexTests, BuildsAndFindsMatches)
{
    int argc = 0;
    char** argv = nullptr;
    std::unique_ptr<QCoreApplication> local;
    if (!QCoreApplication::instance())
        local = std::make_unique<QCoreApplication>(argc, argv);

    ProjectExplorerSearchIndex index;
    index.setEntries(makeEntries());

    QSignalSpy spy(&index, &ProjectExplorerSearchIndex::indexRebuilt);
    ASSERT_TRUE(spy.wait(1000));

    const auto matches = index.findMatches(QStringLiteral("read"));
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches.front(), QStringLiteral("docs/README.md"));
}
