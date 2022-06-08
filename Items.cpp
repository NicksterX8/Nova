#include "Items.hpp"

ItemTypeData ItemData[NUM_ITEMS];

ItemStack::ItemStack() {
    item = 0;
    quantity = 0;
}

ItemStack::ItemStack(Item item):
item(item) {
    quantity = 1;
}

ItemStack::ItemStack(Item item, Uint32 quantity):
item(item), quantity(quantity) {

}

int ItemStack::reduceQuantity(Uint32 reduction) {
    if (quantity == INFINITE_ITEM_QUANTITY) {
        return reduction;
    }
    if (reduction == INFINITE_ITEM_QUANTITY) {
        return quantity;
    }

    int remaining = quantity - reduction;
    if (remaining < 1) {
        quantity = 0;
        item = 0;
        return quantity - remaining;
    }
    quantity -= reduction;
    return reduction;
}