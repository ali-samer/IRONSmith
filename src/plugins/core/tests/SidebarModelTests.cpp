#include <gtest/gtest.h>

#include "core/SidebarModel.hpp"
#include "core/api/SidebarToolSpec.hpp"

#include <QtCore/QString>
#include <QtCore/QVector>

using Core::SidebarFamily;
using Core::SidebarModel;
using Core::SidebarRail;
using Core::SidebarRegion;
using Core::SidebarSide;
using Core::SidebarToolSpec;

namespace {

SidebarToolSpec makeSpec(QString id,
                                int order,
                                SidebarSide side = SidebarSide::Left,
                                SidebarFamily family = SidebarFamily::Vertical,
                                SidebarRegion region = SidebarRegion::Exclusive,
                                SidebarRail rail = SidebarRail::Top)
{
    SidebarToolSpec s;
    s.id = std::move(id);
    s.title = s.id;
    s.iconResource = ":/ui/icons/dummy.svg";
    s.side = side;
    s.family = family;
    s.region = region;
    s.rail = rail;
    s.order = order;
    return s;
}

SidebarModel::PanelFactory dummyFactory()
{
    return [](QWidget*) -> QWidget* { return nullptr; };
}

struct SignalLog
{
    int toolRegistered = 0;
    int toolUnregistered = 0;

    struct RailChange { SidebarSide side; SidebarFamily fam; SidebarRail rail; };
    QVector<RailChange> railChanges;

    struct OpenChange { QString id; bool open; };
    QVector<OpenChange> openChanges;

    struct ActiveChange { SidebarSide side; SidebarFamily fam; SidebarRegion region; QString activeId; };
    QVector<ActiveChange> activeChanges;
};

static void connectSignals(SidebarModel& m, SignalLog& log)
{
    QObject::connect(&m, &SidebarModel::toolRegistered, [&log](const QString&) {
        ++log.toolRegistered;
    });
    QObject::connect(&m, &SidebarModel::toolUnregistered, [&log](const QString&) {
        ++log.toolUnregistered;
    });
    QObject::connect(&m, &SidebarModel::railToolsChanged,
                     [&log](SidebarSide side, SidebarFamily fam, SidebarRail rail) {
                         log.railChanges.push_back({side, fam, rail});
                     });
    QObject::connect(&m, &SidebarModel::toolOpenStateChanged,
                     [&log](const QString& id, bool open) {
                         log.openChanges.push_back({id, open});
                     });
    QObject::connect(&m, &SidebarModel::exclusiveActiveChanged,
                     [&log](SidebarSide side, SidebarFamily fam, SidebarRegion region, const QString& activeId) {
                         log.activeChanges.push_back({side, fam, region, activeId});
                     });
}

} // namespace

TEST(SidebarModelTests, RegisterToolRejectsInvalidId)
{
    SidebarModel m;

    QString err;
    EXPECT_FALSE(m.registerTool(makeSpec("", 0), dummyFactory(), &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.registerTool(makeSpec("   ", 0), dummyFactory(), &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.registerTool(makeSpec("has space", 0), dummyFactory(), &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.registerTool(makeSpec("bad/id", 0), dummyFactory(), &err));
    EXPECT_FALSE(err.isEmpty());
}

TEST(SidebarModelTests, RegisterToolRejectsDuplicateId)
{
    SidebarModel m;

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), dummyFactory(), &err));
    EXPECT_TRUE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.registerTool(makeSpec("project", 1), dummyFactory(), &err));
    EXPECT_FALSE(err.isEmpty());
}

TEST(SidebarModelTests, RegisterToolRejectsEmptyFactory)
{
    SidebarModel m;

    QString err;
    SidebarModel::PanelFactory empty;
    EXPECT_FALSE(m.registerTool(makeSpec("project", 0), empty, &err));
    EXPECT_FALSE(err.isEmpty());
}

TEST(SidebarModelTests, RegistrationEmitsExpectedSignals)
{
    SidebarModel m;
    SignalLog log;
    connectSignals(m, log);

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), dummyFactory(), &err));
    EXPECT_TRUE(err.isEmpty());

    EXPECT_EQ(log.toolRegistered, 1);
    ASSERT_EQ(log.railChanges.size(), 1);
    EXPECT_EQ(int(log.railChanges[0].side), int(SidebarSide::Left));
    EXPECT_EQ(int(log.railChanges[0].fam), int(SidebarFamily::Vertical));
    EXPECT_EQ(int(log.railChanges[0].rail), int(SidebarRail::Top));
}

TEST(SidebarModelTests, OrderingIsDeterministicByOrderThenId)
{
    SidebarModel m;

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("b", 10), dummyFactory(), &err));
    EXPECT_TRUE(m.registerTool(makeSpec("a", 10), dummyFactory(), &err));
    EXPECT_TRUE(m.registerTool(makeSpec("c",  5), dummyFactory(), &err));

    const QVector<QString> ids = m.toolIdsForRail(SidebarSide::Left, SidebarFamily::Vertical, SidebarRail::Top);

    ASSERT_EQ(ids.size(), 3);
    EXPECT_EQ(ids[0], "c"); // order 5
    EXPECT_EQ(ids[1], "a"); // order 10, id a < b
    EXPECT_EQ(ids[2], "b");
}

TEST(SidebarModelTests, UnknownToolRequestsFailWithError)
{
    SidebarModel m;

    QString err;
    EXPECT_FALSE(m.requestShowTool("missing", &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.requestHideTool("missing", &err));
    EXPECT_FALSE(err.isEmpty());

    err.clear();
    EXPECT_FALSE(m.requestToggleTool("missing", &err));
    EXPECT_FALSE(err.isEmpty());
}

TEST(SidebarModelTests, ExclusiveShowSetsActiveAndOpenState)
{
    SidebarModel m;
    SignalLog log;
    connectSignals(m, log);

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0, SidebarSide::Left, SidebarFamily::Vertical,
                                       SidebarRegion::Exclusive, SidebarRail::Top),
                               dummyFactory(), &err));

    EXPECT_TRUE(m.requestShowTool("project", &err));
    EXPECT_TRUE(err.isEmpty());

    EXPECT_TRUE(m.isOpen("project"));
    EXPECT_TRUE(m.isActiveExclusive("project"));
    EXPECT_EQ(m.activeToolId(SidebarSide::Left, SidebarFamily::Vertical, SidebarRegion::Exclusive), "project");

    ASSERT_FALSE(log.activeChanges.isEmpty());
    EXPECT_EQ(log.activeChanges.back().activeId, "project");

    ASSERT_FALSE(log.openChanges.isEmpty());
    EXPECT_EQ(log.openChanges.back().id, "project");
    EXPECT_TRUE(log.openChanges.back().open);
}

TEST(SidebarModelTests, ExclusiveSwitchClosesPreviousAndOpensNew)
{
    SidebarModel m;
    SignalLog log;
    connectSignals(m, log);

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), dummyFactory(), &err));
    EXPECT_TRUE(m.registerTool(makeSpec("structure", 1), dummyFactory(), &err));

    EXPECT_TRUE(m.requestShowTool("project", &err));
    EXPECT_TRUE(m.requestShowTool("structure", &err));

    EXPECT_FALSE(m.isOpen("project"));
    EXPECT_TRUE(m.isOpen("structure"));

    EXPECT_EQ(m.activeToolId(SidebarSide::Left, SidebarFamily::Vertical, SidebarRegion::Exclusive), "structure");

    ASSERT_GE(log.openChanges.size(), 2);
    EXPECT_EQ(log.openChanges[log.openChanges.size() - 2].id, "project");
    EXPECT_FALSE(log.openChanges[log.openChanges.size() - 2].open);
    EXPECT_EQ(log.openChanges.back().id, "structure");
    EXPECT_TRUE(log.openChanges.back().open);
}

TEST(SidebarModelTests, ExclusiveToggleClearsWhenAlreadyActive)
{
    SidebarModel m;

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), dummyFactory(), &err));
    EXPECT_TRUE(m.requestShowTool("project", &err));

    EXPECT_TRUE(m.requestToggleTool("project", &err)); // should clear
    EXPECT_TRUE(err.isEmpty());

    EXPECT_FALSE(m.isOpen("project"));
    EXPECT_TRUE(m.activeToolId(SidebarSide::Left, SidebarFamily::Vertical, SidebarRegion::Exclusive).isEmpty());
}

TEST(SidebarModelTests, AdditiveOpenCloseIsIndependent)
{
    SidebarModel m;

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("find", 0, SidebarSide::Left, SidebarFamily::Vertical,
                                       SidebarRegion::Additive, SidebarRail::Bottom),
                               dummyFactory(), &err));

    EXPECT_FALSE(m.isOpen("find"));

    EXPECT_TRUE(m.requestShowTool("find", &err));
    EXPECT_TRUE(m.isOpen("find"));

    EXPECT_TRUE(m.requestHideTool("find", &err));
    EXPECT_FALSE(m.isOpen("find"));

    EXPECT_TRUE(m.requestToggleTool("find", &err));
    EXPECT_TRUE(m.isOpen("find"));
}


TEST(SidebarModelTests, AdditiveSelectionIsSingleSlotPerSideAndFamily)
{
    SidebarModel m;

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("a1", 0, SidebarSide::Left, SidebarFamily::Vertical,
                                       SidebarRegion::Additive, SidebarRail::Bottom),
                               dummyFactory(), &err));
    EXPECT_TRUE(m.registerTool(makeSpec("a2", 1, SidebarSide::Left, SidebarFamily::Vertical,
                                       SidebarRegion::Additive, SidebarRail::Bottom),
                               dummyFactory(), &err));

    EXPECT_TRUE(m.requestShowTool("a1", &err));
    EXPECT_TRUE(m.isOpen("a1"));
    EXPECT_FALSE(m.isOpen("a2"));

    EXPECT_TRUE(m.requestShowTool("a2", &err));
    EXPECT_FALSE(m.isOpen("a1"));
    EXPECT_TRUE(m.isOpen("a2"));

    EXPECT_TRUE(m.requestToggleTool("a2", &err));
    EXPECT_FALSE(m.isOpen("a2"));
}

TEST(SidebarModelTests, UnregisterClearsActiveAndEmitsClosedIfOpen)
{
    SidebarModel m;
    SignalLog log;
    connectSignals(m, log);

    QString err;
    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), dummyFactory(), &err));
    EXPECT_TRUE(m.requestShowTool("project", &err));
    ASSERT_TRUE(m.isOpen("project"));

    EXPECT_TRUE(m.unregisterTool("project", &err));
    EXPECT_TRUE(err.isEmpty());

    EXPECT_FALSE(m.hasTool("project"));
    EXPECT_TRUE(m.activeToolId(SidebarSide::Left, SidebarFamily::Vertical, SidebarRegion::Exclusive).isEmpty());

    EXPECT_EQ(log.toolUnregistered, 1);
    ASSERT_FALSE(log.activeChanges.isEmpty());
    EXPECT_TRUE(log.activeChanges.back().activeId.isEmpty());
}

TEST(SidebarModelTests, FactoryIsStoredAndRetrievable)
{
    SidebarModel m;

    QString err;
    auto f = dummyFactory();

    EXPECT_TRUE(m.registerTool(makeSpec("project", 0), f, &err));
    EXPECT_TRUE(err.isEmpty());

    auto out = m.panelFactory("project");
    EXPECT_TRUE(bool(out));
}
