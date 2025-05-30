#ifndef ITEMS_MANAGER_INCLUDED
#define ITEMS_MANAGER_INCLUDED

#include "Item.hpp"
#include "ComponentManager.hpp"
#include "components/components.hpp"

namespace items {

// TODO: Improve efficiency by using a custom allocation method
struct InventoryAllocator {
    using SizeT = Sint32;

    //ItemStack* stackBuffer;

    InventoryAllocator() = default;

    InventoryAllocator(SizeT bufferSize) {

    }

    ItemStack* allocate(SizeT size) {
        return Alloc<ItemStack>(size);
    }

    void deallocate(ItemStack* stacks, SizeT size) {
        Free(stacks);
        (void)size;
    }
};

struct ItemManager : PECS::EntityManager {
    InventoryAllocator inventoryAllocator;

    ItemManager() {}

    ItemManager(ComponentInfoRef componentInfo, int numPrototypes)
     : PECS::EntityManager(componentInfo, numPrototypes),
       inventoryAllocator() {

    }
};

using PrototypeManager = PECS::PrototypeManager;

using ItemPrototype = PECS::Prototype;

using PECS::getComponent;
using PECS::getPrototype;
using PECS::addComponent;
using PECS::New;
inline Item makeItem(ItemType type, ItemManager& manager) {
    return Item(PECS::makeEntity(type, manager));
}

inline void freeItem(Item item, ItemManager& manager) {
    // TODO:
    LogInfo("Item should be freed.");
}

void destroyItem(Item* item, ItemManager& manager);
inline void destroyItemStack(ItemStack* stack, ItemManager& manager) {
    destroyItem(&stack->item, manager);
    stack->quantity = 0;
}

inline ItemQuantity getStackSize(Item item, const ItemManager& manager) {
    auto stackSizeComponent = getComponent<ITC::StackSize>(item, manager);
    return stackSizeComponent ? stackSizeComponent->quantity : 0;
}

inline bool isSame(const Item& lhs, const Item& rhs) {
    return (lhs.componentsLoc.archetype == rhs.componentsLoc.archetype)
        && (lhs.componentsLoc.index     == rhs.componentsLoc.index);
}

/* Combine the two item stacks into one
 *
 */
inline bool combineStacks(ItemStack* dst, ItemStack* src, ItemManager& manager) {
    if (!dst || !src) return false;
    if (isSame(src->item, dst->item)) {
        ItemQuantity stackSize = getStackSize(dst->item, manager);
        ItemQuantity totalSize = dst->quantity + src->quantity;
        ItemQuantity itemsToMove = MIN(stackSize, totalSize) - dst->quantity;

        dst->quantity += itemsToMove;
        src->reduceQuantity(itemsToMove);

        return true;
    }
    // not stackable
    return false;
}

} // namespace items

using items::ItemManager;
using items::ItemPrototype;

#endif