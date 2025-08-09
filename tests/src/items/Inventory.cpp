#include <gtest/gtest.h>

#include "items/items.hpp"

using namespace items;

TEST(ItemStack, ReduceQuantity) {
    ItemStack stack = ItemStack(ItemTypes::Grenade, 5);
    ItemQuantity numRemoved = stack.reduceQuantity(4);
    EXPECT_EQ(stack.quantity, 1);
    EXPECT_EQ(numRemoved, 4);
    EXPECT_EQ(stack.reduceQuantity(10), 1);
    EXPECT_EQ(stack.quantity, 0);
    EXPECT_TRUE(stack.empty());

    ItemStack s2{ItemTypes::Grenade, 10};
    EXPECT_EQ(s2.reduceQuantity(10), 10);

    EXPECT_EQ(s2.quantity, 0);
}