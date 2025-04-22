#pragma once

#include "Item.hpp"
#include "Inventory.hpp"
#include "manager.hpp"

namespace items {

int getFirstItemOfType(Inventory& inventory, ItemType type) {
    for (int s = 0; s < inventory.size; s++) {
        if (inventory.type(s) == type) {
            return s;
        }
    }
    return -1;
}

void moveItemStack(Inventory* dst, int dstSlot, Inventory* src, int srcSlot) {
    auto& srcStack = src->get(srcSlot);
    auto& dstStack = dst->get(dstSlot);

    assert(dstStack.empty() && "slot must be empty to be moved to");

    dstStack = srcStack;
    srcStack = ItemStack::None();

    src->size--;
    dst->size++;
}

ItemQuantity addItemStackToInventory(ItemManager& manager, Inventory* dst, ItemStack stack, ItemQuantity itemStackSize) {
    UNFINISHED_CRASH();
    for (int s = 0; s < dst->size; s++) {
        auto slotType = dst->type(s);
        // the second check should become more complicated later on for stuff like durability stacks adding together
        if (isSame(stack.item, dst->get(s).item)) {
            
        }
        if (slotType == stack.item.type && dst->signature(s) == stack.item.signature) {
            /*
            auto maxQuantity = componentManager->getType(slotType)->stackSize;
            auto maxQuantity2 = componentManager->get<ITC::StackSize>()->;
            auto maxQuantity3 = slotType->stackSize;
            */
            
            auto spaceLeft = itemStackSize - dst->quantity(s);
            if (spaceLeft > 0) {
                // blah blah blah
            }
        }
    }
}

}