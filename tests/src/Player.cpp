#include <gtest/gtest.h>
#include "Player.hpp"


TEST(ItemHold, Value) {
    ItemStack value{ItemTypes::Grenade, 4};
    ItemHold itemHold{value};
    bool isSame = items::isSame(itemHold.get()->item, value.item);
    EXPECT_TRUE(isSame);
    EXPECT_EQ(value, *itemHold.get());
    EXPECT_EQ(itemHold.type, ItemHold::Value);
}

TEST(ItemHold, Reference) {
    ItemStack value{ItemTypes::Grenade, 4};
    ItemHold itemHold{ItemHold::Inventory, &value};
    EXPECT_EQ(*itemHold.get(), value);
    EXPECT_EQ(itemHold.type, ItemHold::Inventory);
    EXPECT_EQ(itemHold.get(), &value);
}


class PlayerTest : testing::Test {

};