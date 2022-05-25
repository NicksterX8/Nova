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

ItemStack::ItemStack(Item item, int quantity):
item(item), quantity(quantity) {

}

int ItemStack::reduceQuantity(int reduction) {
    quantity -= reduction;
    if (quantity <= 0) {
        quantity = 0;
        item = 0;
        return quantity;
    }
    return reduction;
}