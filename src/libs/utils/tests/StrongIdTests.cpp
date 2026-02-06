#include <gtest/gtest.h>

#include "utils/StrongId.hpp"

#include <QtCore/QHash>

namespace {

struct WidgetTag final {};
using WidgetId = Utils::StrongId<WidgetTag>;

} // namespace

TEST(StrongIdTests, CreateAndCompare)
{
    const WidgetId a = WidgetId::create();
    const WidgetId b = WidgetId::create();

    EXPECT_FALSE(a.isNull());
    EXPECT_FALSE(b.isNull());
    EXPECT_NE(a, b);

    EXPECT_TRUE(a == a);
    EXPECT_EQ(a.uuid(), a.uuid());
}

TEST(StrongIdTests, FromStringRoundTrip)
{
    const WidgetId id = WidgetId::create();
    const QString s = id.toString(QUuid::WithoutBraces);

    const auto parsed = WidgetId::fromString(s);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->uuid(), id.uuid());

    const auto parsedBraced = WidgetId::fromString(QString("{%1}").arg(s));
    ASSERT_TRUE(parsedBraced.has_value());
    EXPECT_EQ(parsedBraced->uuid(), id.uuid());
}

TEST(StrongIdTests, HashWorksInQHash)
{
    QHash<WidgetId, int> map;
    const WidgetId id = WidgetId::create();

    map.insert(id, 42);
    EXPECT_TRUE(map.contains(id));
    EXPECT_EQ(map.value(id), 42);
}
