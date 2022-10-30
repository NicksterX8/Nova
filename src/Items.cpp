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
    if (quantity == ItemQuantityInfinity) {
        return reduction;
    }
    if (reduction == ItemQuantityInfinity) {
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
    return ItemStack::Empty();
}

ItemQuantity Inventory::addItemStack(ItemStack stack) {
    ItemQuantity itemsLeft = stack.quantity;
    // find first unused slot or first slot of same type and add it to that slot
    for (Uint32 i = 0; i < size && itemsLeft > 0; i++) {
        auto& inventoryStack = get(i);
        if (stack.quantity == ItemQuantityInfinity) {
            if (!inventoryStack.item) {
                inventoryStack = stack;
                return ItemQuantityInfinity;
            }
        }
        // else need to use another slot
        else if (!inventoryStack.item) {
            // add as many items as can fit based on the item's stack size and how many are in the stack
            // if it weren't for the possibility of a stack of items having a quantity higher
            // than the stack's stacksize it would work to just copy the stack straight into the inventory slot
            inventoryStack.item = stack.item;
            ItemQuantity itemsToAdd = (ItemData[stack.item].stackSize < itemsLeft) ? 
                                ItemData[stack.item].stackSize : itemsLeft;
            inventoryStack.quantity = itemsToAdd;
            itemsLeft -= itemsToAdd;
        }
        else if (inventoryStack.item == stack.item) {
            if (inventoryStack.quantity == ItemQuantityInfinity) {
                return stack.quantity;
            }

            int room = ItemData[inventoryStack.item].stackSize - inventoryStack.quantity;
            // if the slot is entirely full this will be <= 0
            // get the minimum of how much items the slot can fit and how many items are in the stack that needs to be added
            if (room > 0) {
                Uint32 itemsToAdd = (itemsLeft < (Uint32)room) ? itemsLeft : (Uint32)room;
                // move as many items as can fit from the stack to the inventory
                inventoryStack.quantity += itemsToAdd;
                itemsLeft -= itemsToAdd;
            }
            // no room left in this slot, continue on
        }
    }
    // couldnt fit atleast some of the stack in the inventory
    return stack.quantity - itemsLeft;
}

ItemQuantity Inventory::removeItemStack(ItemStack stack) {
    ItemQuantity itemsLeft = stack.quantity;
    // go through slots of the same type as the stack and
    // remove as many items as possible from that slot
    for (Uint32 i = 0; i < size; i++) {
        auto& invStack = get(i);
        if (invStack.item == stack.item) {
            // get the minimum of the two quanities, to not remove more than the stack said to,
            // and not to remove more than are in the slot
            Uint32 itemsToRemove = (itemsLeft < invStack.quantity) ? itemsLeft : invStack.quantity;
            itemsLeft -= itemsToRemove;
            if (itemsLeft == 0) {
                break;
            }
        }
    }
    return stack.quantity - itemsLeft;
}

ItemQuantity Inventory::itemCount(Item item) {
    ItemQuantity count = 0;
    for (Uint32 i = 0; i < size; i++) {
        auto& inventoryStack = get(i);
        if (inventoryStack.item == item) {
            if (inventoryStack.infinite()) return ItemQuantityInfinity;
            else count += inventoryStack.quantity;
        }
    }
    return count;
}