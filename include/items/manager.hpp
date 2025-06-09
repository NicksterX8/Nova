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

struct ItemManager : ECS::EntityManager {
    InventoryAllocator inventoryAllocator;

    ItemManager() {}

    ItemManager(ComponentInfoRef componentInfo, int numPrototypes)
     : ECS::EntityManager(componentInfo, numPrototypes),
       inventoryAllocator() {

    }

    Item newItem(ItemType type) {
        Entity entity = this->newEntity(type);
        return Item(type, entity);
    }
};

using PrototypeManager = ECS::PrototypeManager;

using ItemPrototype = ECS::Prototype;

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
    auto stackSizeComponent = manager.getComponent<ITC::StackSize>(item);
    return stackSizeComponent ? stackSizeComponent->quantity : 0;
}

inline bool isSame(const Item& lhs, const Item& rhs) {
    return (lhs.type == rhs.type)
        && (lhs.id == rhs.id)
        && (lhs.version == rhs.version);
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