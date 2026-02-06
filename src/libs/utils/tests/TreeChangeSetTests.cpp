#include <gtest/gtest.h>

#include "utils/TreeChangeSet.hpp"

using Utils::TreeChangeSet;
using Utils::TreeNodeId;

TEST(TreeChangeSetTests, CollectsChanges)
{
    TreeChangeSet changes;
    const TreeNodeId a = TreeNodeId::create();
    const TreeNodeId b = TreeNodeId::create();

    EXPECT_TRUE(changes.empty());
    changes.addAdded(a, TreeNodeId::null(), 0);
    changes.addUpdated(b);
    EXPECT_FALSE(changes.empty());
    EXPECT_EQ(changes.changes().size(), 2);

    changes.clear();
    EXPECT_TRUE(changes.empty());
}
