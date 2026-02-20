// SPDX-FileCopyrightText: 2026 Samer Ali
// SPDX-License-Identifier: GPL-3.0-only

#include <gtest/gtest.h>

#include "projectexplorer/ProjectExplorerTreeState.hpp"
#include "projectexplorer/ProjectExplorerService.hpp"
#include "projectexplorer/api/ProjectExplorerTypes.hpp"

#include <utils/EnvironmentQtPolicy.hpp>

#include <QtCore/QMetaObject>
#include <QtCore/QTemporaryDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeView>

using ProjectExplorer::ProjectEntry;
using ProjectExplorer::ProjectEntryKind;
using ProjectExplorer::ProjectEntryList;
using ProjectExplorer::Internal::ProjectExplorerService;
using ProjectExplorer::Internal::ProjectExplorerTreeState;

namespace {

QApplication* ensureApp()
{
    static QApplication* app = []() {
        static int argc = 1;
        static char arg0[] = "projectexplorer-tests";
        static char* argv[] = { arg0, nullptr };
        return new QApplication(argc, argv);
    }();
    return app;
}

Utils::Environment makeTestEnvironment(const QString& root)
{
    Utils::EnvironmentConfig cfg;
    cfg.organizationName = QStringLiteral("IRONSmith");
    cfg.applicationName = QStringLiteral("IRONSmith");
    cfg.globalConfigRootOverride = root;
    return Utils::Environment(cfg);
}

ProjectEntryList sampleEntries()
{
    ProjectEntryList entries;
    entries.push_back({ QStringLiteral("docs/readme.md"), ProjectEntryKind::Asset });
    entries.push_back({ QStringLiteral("src/main.cpp"), ProjectEntryKind::Asset });
    return entries;
}

} // namespace

TEST(ProjectExplorerTreeStateTests, RestoresExpandedFolders)
{
    ensureApp();

    QTemporaryDir stateDir;
    ASSERT_TRUE(stateDir.isValid());

    QTemporaryDir rootDir;
    ASSERT_TRUE(rootDir.isValid());

    ProjectExplorerService service;
    service.setEntries(sampleEntries());
    service.setRootPath(rootDir.path(), false);

    QTreeView view;
    view.setModel(service.model());

    ProjectExplorerTreeState state(&service, makeTestEnvironment(stateDir.path()));
    state.attach(&view);
    state.setRootPath(rootDir.path(), false);

    const QModelIndex docs = service.indexForPath(QStringLiteral("docs"));
    ASSERT_TRUE(docs.isValid());

    view.setExpanded(docs, true);
    QMetaObject::invokeMethod(&state, "flushSave", Qt::DirectConnection);

    QTreeView view2;
    view2.setModel(service.model());

    ProjectExplorerTreeState state2(&service, makeTestEnvironment(stateDir.path()));
    state2.attach(&view2);
    state2.setRootPath(rootDir.path(), false);

    const QModelIndex docs2 = service.indexForPath(QStringLiteral("docs"));
    ASSERT_TRUE(docs2.isValid());
    EXPECT_TRUE(view2.isExpanded(docs2));
}

TEST(ProjectExplorerTreeStateTests, UserInitiatedRootAutoExpands)
{
    ensureApp();

    QTemporaryDir stateDir;
    ASSERT_TRUE(stateDir.isValid());

    QTemporaryDir rootB;
    ASSERT_TRUE(rootB.isValid());

    ProjectExplorerService service;
    service.setEntries(sampleEntries());
    service.setRootPath(rootB.path(), false);

    QTreeView view;
    view.setModel(service.model());

    ProjectExplorerTreeState state(&service, makeTestEnvironment(stateDir.path()));
    state.attach(&view);

    state.setRootPath(rootB.path(), false);
    const QModelIndex rootIndex = view.model()->index(0, 0, QModelIndex());
    ASSERT_TRUE(rootIndex.isValid());

    view.setExpanded(rootIndex, false);
    QMetaObject::invokeMethod(&state, "flushSave", Qt::DirectConnection);

    ProjectExplorerTreeState state2(&service, makeTestEnvironment(stateDir.path()));
    state2.attach(&view);
    state2.setRootPath(rootB.path(), true);

    EXPECT_TRUE(view.isExpanded(rootIndex));
}
