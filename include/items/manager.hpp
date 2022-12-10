#ifndef ITEMS_MANAGER_INCLUDED
#define ITEMS_MANAGER_INCLUDED

#include "Item.hpp"
#include "ComponentManager.hpp"

namespace items {

// TODO: Improve efficiency by using a custom allocation method
struct InventoryAllocator {
    using SizeT = Sint32;

    ItemStack* stackBuffer;

    ItemStack* allocate(SizeT size) {
        return Alloc<ItemStack>(size);
    }

    void deallocate(ItemStack* stacks, SizeT size) {
        Free(stacks);
        (void)size;
    }
};

struct ItemManager {
    ComponentManager    componentManager;
    PrototypeManager    prototypeManager;
    InventoryAllocator  inventoryAllocator;

    ItemManager() {}

    template<class C>
    C* get() const {
        return getComponent<C>(&componentManager, )
    }

    bool stackable(ItemStack lhs, ItemStack rhs) const {
        return (lhs.item.type == rhs.item.type && lhs.item.signature == rhs.item.signature);
    }

    ItemStack combineStacks(ItemStack lhs, ItemStack rhs) const {
        return {lhs.item, lhs.quantity + rhs.quantity};
    }

    ItemQuantity addItemStackToInventory(Inventory* dst, ItemStack stack, ItemQuantity itemStackSize) {
        for (int s = 0; s < dst->size; s++) {
            auto slotType = dst->type(s);
            // the second check should become more complicated later on for stuff like durability stacks adding together
            if (stackable(stack, dst->get(s))) {
                
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
};

} // namespace items

using items::ItemManager;

#endif