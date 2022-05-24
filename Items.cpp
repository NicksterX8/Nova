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

Inventory::Inventory(int inventorySize) {
    if (inventorySize < 9) {
        inventorySize = 9;
    }
    this->size = inventorySize;
}

int Inventory::addItemStack(ItemStack stack) {
    int itemsTransferred = 0;
    // find first unused slot or first slot of same type and add it to that slot
    for (int i = 0; i < size; i++) {
        if (!items[i].item) {
            // completely open slot, just copy the whole stack in.
            items[i] = stack;
            // We could set itemsTransferred = stack.quantity and break out of the loop,
            // but this has the same effect and a bit easier.
            return stack.quantity;
        }
        else if (items[i].item == stack.item) {
            int room = ItemData[items[i].item].stackSize - items[i].quantity;
            // if the slot is entirely full this will be <= 0
            // get the minimum of how much items the slot can fit and how many items are in the stack that needs to be added
            if (room > 0) {
                int itemsToAdd = stack.quantity;
                if (itemsToAdd > room) {
                    itemsToAdd = room;
                }
                // move as many items as can fit from the stack to the inventory
                items[i].quantity += itemsToAdd;
                stack.quantity -= itemsToAdd;
                itemsTransferred += itemsToAdd;
                if (stack.quantity == 0) {
                    // could fit the entire stack into one slot
                    return itemsTransferred;
                } else {
                    // else need to use another slot because there isn't enough room to fit all the items in the stack
                    continue;
                }
            } else {
                // no room in this slot, continue on
            }
        }
    }
    // couldnt fit atleast some of the stack in the inventory
    return itemsTransferred;
}