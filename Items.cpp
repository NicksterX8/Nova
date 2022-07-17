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

ItemStack Inventory::firstItemStack() {
    for (Uint32 i = 0; i < size; i++) {
        if (get(i).item) {
            return get(i).item;
        }
    }
    return ItemStack();
}

Uint32 Inventory::addItemStack(ItemStack stack) {
    Uint32 itemsLeft = stack.quantity;
    // find first unused slot or first slot of same type and add it to that slot
    for (Uint32 i = 0; i < size && itemsLeft > 0; i++) {
        if (stack.quantity == INFINITE_ITEM_QUANTITY) {
            if (!get(i).item) {
                get(i) = stack;
                return INFINITE_ITEM_QUANTITY;
            }
        }
        // else need to use another slot
        else if (!get(i).item) {
            // add as many items as can fit based on the item's stack size and how many are in the stack
            // if it weren't for the possibility of a stack of items having a quantity higher
            // than the stack's stacksize it would work to just copy the stack straight into the inventory slot
            get(i).item = stack.item;
            Uint32 itemsToAdd = (ItemData[stack.item].stackSize < itemsLeft) ? 
                                ItemData[stack.item].stackSize : itemsLeft;
            get(i).quantity = itemsToAdd;
            itemsLeft -= itemsToAdd;
        }
        else if (get(i).item == stack.item) {
            if (get(i).quantity == INFINITE_ITEM_QUANTITY) {
                return stack.quantity;
            }

            int room = ItemData[get(i).item].stackSize - get(i).quantity;
            // if the slot is entirely full this will be <= 0
            // get the minimum of how much items the slot can fit and how many items are in the stack that needs to be added
            if (room > 0) {
                Uint32 itemsToAdd = (itemsLeft < (Uint32)room) ? itemsLeft : (Uint32)room;
                // move as many items as can fit from the stack to the inventory
                get(i).quantity += itemsToAdd;
                itemsLeft -= itemsToAdd;
                
            }
            // no room left in this slot, continue on
        }
    }
    // couldnt fit atleast some of the stack in the inventory
    return stack.quantity - itemsLeft;
}

Uint32 Inventory::removeItemStack(ItemStack stack) {
    Uint32 itemsLeft = stack.quantity;
    // go through slots of the same type as the stack and
    // remove as many items as possible from that slot
    for (Uint32 i = 0; i < size; i++) {
        if (get(i).item == stack.item) {
            // get the minimum of the two quanities, to not remove more than the stack said to,
            // and not to remove more than are in the slot
            Uint32 itemsToRemove = (itemsLeft < get(i).quantity) ? itemsLeft : get(i).quantity;
            itemsLeft -= itemsToRemove;
            if (itemsLeft == 0) {
                break;
            }
        }
    }
    return stack.quantity - itemsLeft;
}

Uint32 Inventory::itemCount(Item item) {
    Uint32 count = 0;
    for (Uint32 i = 0; i < size; i++) {
        if (get(i).item == item) {
            count += get(i).quantity;
        }
    }
    return count;
}