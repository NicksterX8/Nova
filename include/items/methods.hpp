#pragma once

#include "Item.hpp"
#include "Inventory.hpp"

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
    srcStack = ItemStack::Empty();

    src->size--;
    dst->size++;
}

}