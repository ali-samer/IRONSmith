#include <gtest/gtest.h>

#include "designmodel/DesignSchemaVersion.hpp"
#include "designmodel/DesignMetadata.hpp"

#include <QtCore/QHash>

using namespace DesignModel;

TEST(DesignSchemaVersion, Current_IsValidAndSupported) {
    const auto v = DesignSchemaVersion::current();
    EXPECT_TRUE(v.isValid());
    EXPECT_TRUE(v.isSupported());
    EXPECT_FALSE(v.requiresMigration());
}

TEST(DesignSchemaVersion, Parse_RoundTrip) {
    const auto v = DesignSchemaVersion{7};
    const auto parsed = DesignSchemaVersion::fromString(v.toString());
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->value(), 7u);
}

TEST(DesignSchemaVersion, Parse_LenientVPrefix) {
    const auto parsed = DesignSchemaVersion::fromString(" v12 ");
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->value(), 12u);
}

TEST(DesignSchemaVersion, Parse_RejectsInvalid) {
    EXPECT_FALSE(DesignSchemaVersion::fromString("").has_value());
    EXPECT_FALSE(DesignSchemaVersion::fromString("v").has_value());
    EXPECT_FALSE(DesignSchemaVersion::fromString("0").has_value());
    EXPECT_FALSE(DesignSchemaVersion::fromString("-1").has_value());
    EXPECT_FALSE(DesignSchemaVersion::fromString("not-a-number").has_value());
}

TEST(DesignSchemaVersion, Hash_WorksInQHash) {
    QHash<DesignSchemaVersion, int> map;
    map.insert(DesignSchemaVersion{1}, 10);
    map.insert(DesignSchemaVersion{2}, 20);

    EXPECT_EQ(map.value(DesignSchemaVersion{1}), 10);
    EXPECT_EQ(map.value(DesignSchemaVersion{2}), 20);
}

TEST(DesignMetadata, NormalizesToUtc) {
    QDateTime local(QDate(2025, 1, 1), QTime(12, 0, 0), Qt::LocalTime);

    DesignMetadata md("n", "a", local);
    EXPECT_TRUE(md.createdUtc().isValid());
    EXPECT_EQ(md.createdUtc().timeSpec(), Qt::UTC);
    EXPECT_TRUE(md.isValid());
}

TEST(DesignMetadata, StoresFields) {
    const QDateTime utc(QDate(2025, 1, 1), QTime(0, 0, 0), Qt::UTC);

    DesignMetadata md("Design1", "Joe", utc, "notes", "profile:foo");
    EXPECT_EQ(md.name(), "Design1");
    EXPECT_EQ(md.author(), "Joe");
    EXPECT_EQ(md.notes(), "notes");
    EXPECT_EQ(md.profileSignature(), "profile:foo");
    EXPECT_EQ(md.createdUtc().timeSpec(), Qt::UTC);
    EXPECT_TRUE(md.isValid());
}

TEST(DesignMetadata, InvalidWhenCreatedInvalid) {
    DesignMetadata md("x", "y", QDateTime{});
    EXPECT_FALSE(md.isValid());
}
