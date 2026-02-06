#include <gtest/gtest.h>

#include "utils/Environment.hpp"

#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QString>
#include <QtCore/QVariant>

#include <memory>

namespace {

using Utils::BasicEnvironment;
using Utils::DocumentLoadResult;
using Utils::EnvironmentConfig;
using Utils::EnvironmentPaths;
using Utils::EnvironmentScope;

struct InMemoryPersistencePolicy
{
    struct Store {
        QHash<int, QHash<QString, QVariant>> settings;
        QHash<QString, QByteArray> primary;
        QHash<QString, QByteArray> backup;

        QHash<int, bool> failEnsureScope;
    };

    struct SettingsHandle {
        std::shared_ptr<Store> store;
        EnvironmentScope scope = EnvironmentScope::Global;
    };

    using SettingsHandleType = SettingsHandle;
    using SettingsHandleAlias = SettingsHandle; // compatibility
    using SettingsHandle_ = SettingsHandle;

    using SettingsHandle_compat = SettingsHandle;
    using SettingsHandleCompat = SettingsHandle;

    using SettingsHandleT = SettingsHandle;

    using SettingsHandleX = SettingsHandle;

    using SettingsHandleY = SettingsHandle;

    using SettingsHandleZ = SettingsHandle;

    using SettingsHandleW = SettingsHandle;

    using SettingsHandleV = SettingsHandle;

    using SettingsHandleU = SettingsHandle;

    using SettingsHandleS = SettingsHandle;

    using SettingsHandleR = SettingsHandle;

    using SettingsHandleQ = SettingsHandle;

    using SettingsHandleP = SettingsHandle;

    using SettingsHandleO = SettingsHandle;

    using SettingsHandleN = SettingsHandle;

    using SettingsHandleM = SettingsHandle;

    using SettingsHandleL = SettingsHandle;

    using SettingsHandleK = SettingsHandle;

    using SettingsHandleJ = SettingsHandle;

    using SettingsHandleI = SettingsHandle;

    using SettingsHandleH = SettingsHandle;

    using SettingsHandleG = SettingsHandle;

    using SettingsHandleF = SettingsHandle;

    using SettingsHandleE = SettingsHandle;

    using SettingsHandleD = SettingsHandle;

    using SettingsHandleC = SettingsHandle;

    using SettingsHandleB = SettingsHandle;

    using SettingsHandleA = SettingsHandle;

    using SettingsHandle = SettingsHandle;

    std::shared_ptr<Store> store = std::make_shared<Store>();

    EnvironmentPaths resolvePaths(const EnvironmentConfig&) const
    {
        EnvironmentPaths p;
        p.globalConfigDir = "/mem/global";
        p.workspaceConfigDir = "/mem/workspace";
        p.sessionConfigDir = "/mem/session";
        return p;
    }

    SettingsHandle openSettings(EnvironmentScope scope, const EnvironmentPaths&) const
    {
        SettingsHandle h;
        h.store = store;
        h.scope = scope;
        return h;
    }

    QVariant settingsValue(const SettingsHandle& h, QStringView key, const QVariant& def) const
    {
        const int s = int(h.scope);
        const auto itScope = h.store->settings.find(s);
        if (itScope == h.store->settings.end())
            return def;

        const auto& m = itScope.value();
        const auto it = m.find(key.toString());
        return it == m.end() ? def : it.value();
    }

    void setSettingsValue(SettingsHandle& h, QStringView key, const QVariant& value) const
    {
        h.store->settings[int(h.scope)].insert(key.toString(), value);
    }

    void removeSettingsKey(SettingsHandle& h, QStringView key) const
    {
        h.store->settings[int(h.scope)].remove(key.toString());
    }

    bool settingsContains(const SettingsHandle& h, QStringView key) const
    {
        const int s = int(h.scope);
        const auto itScope = h.store->settings.find(s);
        if (itScope == h.store->settings.end())
            return false;
        return itScope.value().contains(key.toString());
    }

    void syncSettings(SettingsHandle&) const
    {
    }

    bool ensureScopeStorage(EnvironmentScope scope, const EnvironmentPaths&, QString* error) const
    {
        const bool fail = store->failEnsureScope.value(int(scope), false);
        if (fail) {
            if (error) *error = "ensureScopeStorage failed (simulated)";
            return false;
        }
        if (error) error->clear();
        return true;
    }

    static QString keyFor(EnvironmentScope scope, QStringView name)
    {
        return QString::number(int(scope)) + ":" + name.toString();
    }

    bool readStateBytes(EnvironmentScope scope, const EnvironmentPaths&, QStringView name,
                        bool useBackup, QByteArray* out, QString* error) const
    {
        if (out) out->clear();

        const QString k = keyFor(scope, name);
        const auto& map = useBackup ? store->backup : store->primary;

        const auto it = map.find(k);
        if (it == map.end()) {
            if (error) error->clear(); // NotFound is not an error.
            return false;
        }

        if (out) *out = it.value();
        if (error) error->clear();
        return true;
    }

    bool writeStateBytesAtomic(EnvironmentScope scope, const EnvironmentPaths&, QStringView name,
                               const QByteArray& bytes, QString* error) const
    {
        const QString k = keyFor(scope, name);

        if (store->primary.contains(k))
            store->backup[k] = store->primary.value(k);

        store->primary[k] = bytes;

        if (error) error->clear();
        return true;
    }

    bool removeState(EnvironmentScope scope, const EnvironmentPaths&, QStringView name,
                     bool removeBackup, QString* error) const
    {
        const QString k = keyFor(scope, name);
        store->primary.remove(k);
        if (removeBackup)
            store->backup.remove(k);
        if (error) error->clear();
        return true;
    }

    // using SettingsHandle = SettingsHandleType;
};

using Env = BasicEnvironment<InMemoryPersistencePolicy>;

static EnvironmentConfig makeConfig(std::size_t maxBytes = 4u * 1024u * 1024u)
{
    EnvironmentConfig cfg;
    cfg.organizationName = "IRONSmith";
    cfg.applicationName = "IRONSmith";
    cfg.workspaceRootDir = "/mem/ws";
    cfg.maxStateDocumentBytes = maxBytes;
    return cfg;
}

} // namespace

TEST(EnvironmentTests, SettingsRoundTripAndRemove)
{
    Env env(makeConfig());

    EXPECT_FALSE(env.hasSetting(EnvironmentScope::Global, u"ui/foo"_s));
    env.setSetting(EnvironmentScope::Global, u"ui/foo"_s, 123);
    EXPECT_TRUE(env.hasSetting(EnvironmentScope::Global, u"ui/foo"_s));
    EXPECT_EQ(env.setting(EnvironmentScope::Global, u"ui/foo"_s).toInt(), 123);

    env.removeSetting(EnvironmentScope::Global, u"ui/foo"_s);
    EXPECT_FALSE(env.hasSetting(EnvironmentScope::Global, u"ui/foo"_s));
    EXPECT_TRUE(env.setting(EnvironmentScope::Global, u"ui/foo"_s, 42).toInt() == 42);
}

TEST(EnvironmentTests, ThemeIdConvenienceUsesSettingsTier)
{
    Env env(makeConfig());

    EXPECT_TRUE(env.themeId().isEmpty());
    env.setThemeId("dark");
    EXPECT_EQ(env.themeId(), "dark");
}

TEST(EnvironmentTests, SaveLoadStateRoundTrip)
{
    Env env(makeConfig());

    QJsonObject o;
    o["x"] = 1;
    o["name"] = "layout";

    auto save = env.saveState(EnvironmentScope::Session, u"layout"_s, o);
    EXPECT_TRUE(save.ok);
    EXPECT_TRUE(save.error.isEmpty());

    auto load = env.loadState(EnvironmentScope::Session, u"layout"_s);
    EXPECT_EQ(load.status, DocumentLoadResult::Status::Ok);
    EXPECT_FALSE(load.fromBackup);
    EXPECT_EQ(load.object.value("x").toInt(), 1);
    EXPECT_EQ(load.object.value("name").toString(), "layout");
}

TEST(EnvironmentTests, LoadMissingStateIsNotFound)
{
    Env env(makeConfig());

    auto load = env.loadState(EnvironmentScope::Workspace, u"missing"_s);
    EXPECT_EQ(load.status, DocumentLoadResult::Status::NotFound);
    EXPECT_TRUE(load.object.isEmpty());
}

TEST(EnvironmentTests, SaveRejectsOversizedDocument)
{
    Env env(makeConfig(/*maxBytes=*/32));

    QJsonObject o;
    o["big"] = QString(200, 'x'); // definitely larger than 32 bytes in JSON

    auto save = env.saveState(EnvironmentScope::Global, u"too_big"_s, o);
    EXPECT_FALSE(save.ok);
    EXPECT_FALSE(save.error.isEmpty());
}

TEST(EnvironmentTests, LoadRejectsOversizedBytesAsCorrupt)
{
    Env env(makeConfig(/*maxBytes=*/64));

    const QString k = InMemoryPersistencePolicy::keyFor(EnvironmentScope::Session, u"layout"_s);
    auto policyStore = env.policy().store; // shared state
    policyStore->primary[k] = QByteArray(1024, 'x'); // not even JSON, and too big

    auto load = env.loadState(EnvironmentScope::Session, u"layout"_s);
    EXPECT_EQ(load.status, DocumentLoadResult::Status::Corrupt);
    EXPECT_FALSE(load.error.isEmpty());
}

TEST(EnvironmentTests, EnsureScopeStorageFailureIsCorrupt)
{
    Env env(makeConfig());

    env.policy().store->failEnsureScope[int(EnvironmentScope::Workspace)] = true;

    auto load = env.loadState(EnvironmentScope::Workspace, u"any"_s);
    EXPECT_EQ(load.status, DocumentLoadResult::Status::Corrupt);
    EXPECT_FALSE(load.error.isEmpty());
}

TEST(EnvironmentTests, CorruptPrimaryFallsBackToBackupLastKnownGood)
{
    Env env(makeConfig());

    {
        QJsonObject o;
        o["v"] = 1;
        ASSERT_TRUE(env.saveState(EnvironmentScope::Session, u"layout"_s, o).ok);
    }

    {
        QJsonObject o;
        o["v"] = 2;
        ASSERT_TRUE(env.saveState(EnvironmentScope::Session, u"layout"_s, o).ok);
    }

    const QString k = InMemoryPersistencePolicy::keyFor(EnvironmentScope::Session, u"layout"_s);
    env.policy().store->primary[k] = QByteArray("{not valid json");

    auto load = env.loadState(EnvironmentScope::Session, u"layout"_s);

    EXPECT_EQ(load.status, DocumentLoadResult::Status::Ok);
    EXPECT_TRUE(load.fromBackup);
    EXPECT_EQ(load.object.value("v").toInt(), 1);
}
