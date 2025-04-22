#ifndef ITEMS_MANAGER_INCLUDED
#define ITEMS_MANAGER_INCLUDED

#include "Item.hpp"
#include "ComponentManager.hpp"
#include "Prototype.hpp"
//#include "Inventory.hpp"

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

struct ItemManager {
    ComponentManager    components;
    PrototypeManager    prototypes;
    InventoryAllocator  inventoryAllocator;

    ItemManager() = default;

    ItemManager(ComponentInfoRef componentInfo)
     : components(componentInfo), prototypes(componentInfo, ItemTypes::Count), inventoryAllocator(32) {} 
};

inline void freeItem(Item item, ItemManager& manager) {
    // TODO:
}

inline const ItemPrototype* getPrototype(ItemType type, const ItemManager& manager) {
    //TODO: check for null
    if (type >= manager.prototypes.prototypes.size()) return nullptr;
    return &manager.prototypes.prototypes[type];
}

template<class C>
const C* getProtoComponent(const Item& item, const ItemManager& manager) {
    // TODO: check for null
    return getPrototype(item.type, manager)->get<C>();
}

template<class C>
C* getRegularComponent(const Item& item, const ItemManager& manager) {
    return static_cast<C*>(manager.components.getComponent(item.componentsLoc, C::ID));
}

template<class C>
typename std::conditional<C::PROTOTYPE, const C*, C*>::type getComponent(const Item& item, const ItemManager& manager) {
    if constexpr (C::PROTOTYPE) {
        return getProtoComponent<C>(item, manager);
    } else {
        return getRegularComponent<C>(item, manager);
    }
}

template<class C>
void setComponent(const Item& item, const ItemManager& manager, const C& value) {
    static_assert(C::PROTOTYPE == false);
    C* component = getComponent<C>(item, manager);
    if (component) {
        *component = value;
    } else {
        LogError("Component %s does not exist for item!", manager.components.componentInfo.name(C::ID));
    }
}

inline Item makeItem(ItemType type, ItemManager& manager) {
    auto* prototype = getPrototype(type, manager);
    if (!prototype) {
        LogError("NO prototype found for item type %d!", type);
        return Item::None();
    }
    ComponentSignature prototypeSignature = 0;
    if (prototype) {
        prototypeSignature = prototype->signature;
    }
    return Item(type, prototypeSignature, NullComponentsAddress);
}

void destroyItem(Item* item, ItemManager& manager);
inline void destroyItemStack(ItemStack* stack, ItemManager& manager) {
    destroyItem(&stack->item, manager);
    stack->quantity = 0;
}

template<class C>
bool addComponent(Item& item, ItemManager& manager, const C& startValue) {
    static_assert(C::PROTOTYPE == false);
    manager.components.addComponent(&item.componentsLoc, C::ID);
    C* component = (C*)manager.components.getComponent(item.componentsLoc, C::ID);
    if (component) {
        *component = startValue;
        item.signature.set(C::ID);
        return true;
    }
    return false;
}

template<class C>
void addComponent(Item& item, ItemManager& manager) {
    static_assert(C::PROTOTYPE == false);
    manager.components.addComponent(item.componentsLoc, C::ID);
    item.signature.set(C::ID);
}

inline ItemQuantity getStackSize(Item item, const ItemManager& manager) {
    auto prototype = getPrototype(item.type, manager);
    assert(prototype && "no prototype for item!");
    return prototype->stackSize;
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

#endif