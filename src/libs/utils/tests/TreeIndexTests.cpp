#include <gtest/gtest.h>

#include "utils/TreeIndex.hpp"

using Utils::TreeIndex;
using Utils::TreeNodeId;

TEST(TreeIndexTests, CreateAndReparent)
{
    TreeIndex<QString> tree;

    const TreeNodeId root = tree.createRoot("root");
    const TreeNodeId a = tree.addChild(root, "a");
    const TreeNodeId b = tree.addChild(root, "b");

    ASSERT_TRUE(tree.contains(a));
    ASSERT_TRUE(tree.contains(b));
    EXPECT_EQ(tree.children(root).size(), 2);
    EXPECT_EQ(tree.childIndex(root, b), 1);

    EXPECT_TRUE(tree.move(b, a));
    EXPECT_EQ(tree.childIndex(root, b), -1);
    EXPECT_EQ(tree.childIndex(a, b), 0);
}

TEST(TreeIndexTests, RemoveSubtree)
{
    TreeIndex<int> tree;
    const TreeNodeId root = tree.createRoot(0);
    const TreeNodeId a = tree.addChild(root, 1);
    const TreeNodeId b = tree.addChild(a, 2);

    EXPECT_TRUE(tree.removeSubtree(a));
    EXPECT_FALSE(tree.contains(a));
    EXPECT_FALSE(tree.contains(b));
    EXPECT_EQ(tree.size(), 1);
    EXPECT_EQ(tree.rootId(), root);
}
