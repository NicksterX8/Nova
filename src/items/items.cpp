#include "items/items.hpp"
#include "items/prototypes/prototypes.hpp"

using namespace items;

ItemQuantity ItemStack::reduceQuantity(ItemQuantity reduction) {
    ItemQuantity oldQuantity = this->quantity;
    quantity = oldQuantity - reduction;
    if (reduction == ItemQuantityInfinity) goto remove_whole_stack;
    else if (oldQuantity == ItemQuantityInfinity) return reduction;
    else if (quantity < 1) {
remove_whole_stack:
        *this = ItemStack::None();
        return oldQuantity;
    } else {
        return MIN(reduction, oldQuantity);
    }
}

ItemStack Inventory::takeFirstItemStack() {
    for (Uint32 i = 0; i < size; i++) {
        auto* stack = &get(i);
        if (!stack->empty()) {
            auto ret = *stack;
            *stack = ItemStack::None();
            return ret;
        }
    }
    return ItemStack::None();
}

ItemQuantity Inventory::addItemStack(ItemStack stack) {
    ItemQuantity itemsLeft = stack.quantity;
    const ItemQuantity stackSize = getStackSize(stack.item, *manager);
    // find first unused slot or first slot of same type and add it to that slot
    for (Uint32 i = 0; i < size && itemsLeft > 0; i++) {
        auto& inventoryStack = get(i);
        if (stack.quantity == ItemQuantityInfinity) {
            if (!inventoryStack.item.type) {
                inventoryStack = stack;
                return ItemQuantityInfinity;
            }
        }
        // else need to use another slot
        else if (inventoryStack.empty()) {
            // add as many items as can fit based on the item's stack size and how many are in the stack
            // if it weren't for the possibility of a stack of items having a quantity higher
            // than the stack's stacksize it would work to just copy the stack straight into the inventory slot


            ItemQuantity itemsToAdd = (stackSize < itemsLeft) ? 
                                stackSize : itemsLeft;
            inventoryStack.quantity = itemsToAdd;
            inventoryStack.item = stack.item;
            itemsLeft -= itemsToAdd;
        }
        else if (isSame(inventoryStack.item, stack.item)) {
            if (inventoryStack.quantity == ItemQuantityInfinity) {
                return stack.quantity;
            }

            int room = getStackSize(inventoryStack.item, *manager) - inventoryStack.quantity;
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

ItemQuantity Inventory::removeItemType(ItemType type, ItemQuantity quantity) {
    ItemQuantity itemsLeft = quantity;
    // go through slots of the same type as the stack and
    // remove as many items as possible from that slot
    if (quantity < 1) {
        if (quantity == ItemQuantityInfinity) {
            // remove all items of the type
            for (Uint32 i = 0; i < size; i++) {
                auto& slot = get(i);
                if (slot.item.type == type) {
                    destroyItem(&slot.item, *manager);
                }
            }
        }
        return quantity;
    }
    // search through each inventory slot and 
    for (Uint32 i = 0; i < size; i++) {
        auto& slot = get(i);
        if (slot.item.type == type) {
            if (slot.quantity == ItemQuantityInfinity) return quantity;
            // get the minimum of the two quanities, to not remove more than the stack said to,
            // and not to remove more than are in the slot
            Uint32 itemsToRemove = MIN(itemsLeft, slot.quantity);

            if (slot.quantity > itemsLeft) {
                slot.quantity -= itemsLeft;
                return quantity;
            } else if (slot.quantity <= itemsLeft) {
                itemsLeft -= slot.quantity;
                freeItem(slot.item, *manager);
                slot = Item::None();
                if (itemsLeft == 0) return quantity;
            }
        }
    }
    return quantity - itemsLeft;
}

ItemQuantity Inventory::itemCount(ItemType type) {
    ItemQuantity count = 0;
    for (Uint32 i = 0; i < size; i++) {
        auto& inventoryStack = get(i);
        if (inventoryStack.item.type == type) {
            if (inventoryStack.infinite()) return ItemQuantityInfinity;
            else count += inventoryStack.quantity;
        }
    }
    return count;
}

void items::destroyItem(Item* item, ItemManager& manager) {
    freeItem(*item, manager);
    *item = Item::None();
}