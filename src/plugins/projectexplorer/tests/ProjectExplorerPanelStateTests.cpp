#include <gtest/gtest.h>

#include "projectexplorer/state/ProjectExplorerPanelState.hpp"
#include "projectexplorer/ProjectExplorerService.hpp"
#include "projectexplorer/api/ProjectExplorerTypes.hpp"

#include <utils/EnvironmentQtPolicy.hpp>
#include <utils/ui/SidebarPanelFrame.hpp>

#include <QtCore/QMetaObject>
#include <QtCore/QTemporaryDir>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTreeView>
#include <QtTest/QSignalSpy>

using ProjectExplorer::ProjectEntry;
using ProjectExplorer::ProjectEntryKind;
using ProjectExplorer::ProjectEntryList;
using ProjectExplorer::Internal::ProjectExplorerPanelState;
using ProjectExplorer::Internal::ProjectExplorerService;

namespace {

QApplication* ensureApp()
{
    static QApplication* app = []() {
        static int argc = 1;
        static char arg0[] = "projectexplorer-panelstate-tests";
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

TEST(ProjectExplorerPanelStateTests, RestoresSelectionForRoot)
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

    Utils::SidebarPanelFrame frame;
    frame.setViewOptions({ QStringLiteral("Project") });
    frame.setTitle(QStringLiteral("Project"));

    ProjectExplorerPanelState state(&service, makeTestEnvironment(stateDir.path()));
    state.attach(&view, &frame);
    state.setRootPath(rootDir.path());

    const QModelIndex docs = service.indexForPath(QStringLiteral("docs"));
    ASSERT_TRUE(docs.isValid());
    view.setCurrentIndex(docs);

    QMetaObject::invokeMethod(&state, "flushSave", Qt::DirectConnection);

    QTreeView view2;
    view2.setModel(service.model());

    Utils::SidebarPanelFrame frame2;
    frame2.setViewOptions({ QStringLiteral("Project") });
    frame2.setTitle(QStringLiteral("Project"));

    ProjectExplorerPanelState state2(&service, makeTestEnvironment(stateDir.path()));
    state2.attach(&view2, &frame2);

    QSignalSpy spy(&service, &ProjectExplorerService::selectPathRequested);
    state2.setRootPath(rootDir.path());

    ASSERT_EQ(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    ASSERT_EQ(args.size(), 1);
    EXPECT_EQ(args.at(0).toString(), QStringLiteral("docs"));
}

TEST(ProjectExplorerPanelStateTests, RestoresViewSelection)
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

    Utils::SidebarPanelFrame frame;
    frame.setViewOptions({ QStringLiteral("Project"), QStringLiteral("Project Files") });
    frame.setTitle(QStringLiteral("Project"));

    ProjectExplorerPanelState state(&service, makeTestEnvironment(stateDir.path()));
    state.attach(&view, &frame);
    state.setRootPath(rootDir.path());

    QMetaObject::invokeMethod(&state, "handleViewSelected",
                              Qt::DirectConnection,
                              Q_ARG(QString, QStringLiteral("Project Files")));
    QMetaObject::invokeMethod(&state, "flushSave", Qt::DirectConnection);

    QTreeView view2;
    view2.setModel(service.model());

    Utils::SidebarPanelFrame frame2;
    frame2.setViewOptions({ QStringLiteral("Project"), QStringLiteral("Project Files") });
    frame2.setTitle(QStringLiteral("Project"));

    ProjectExplorerPanelState state2(&service, makeTestEnvironment(stateDir.path()));
    state2.attach(&view2, &frame2);
    state2.setRootPath(rootDir.path());

    EXPECT_EQ(frame2.title(), QStringLiteral("Project Files"));
}
