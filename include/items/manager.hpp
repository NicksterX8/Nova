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

inline bool stackable(ItemStack lhs, ItemStack rhs, const ItemManager& manager) {
    return (lhs.item.type == rhs.item.type && lhs.item.signature == rhs.item.signature);
}

/* Combine the two item stacks into one
 *
 */
inline ItemStack combineStacks(ItemStack dst, ItemStack src, ItemManager& manager) {
    if (src.item.type == dst.item.type && src.item.signature == dst.item.signature) {
        freeItem(src.item, manager);
        return ItemStack(dst.item, dst.quantity + src.quantity);
    }
    // not stackable
    return ItemStack::None();
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
    if (C::PROTOTYPE) {
        return getProtoComponent<C>(item, manager);
    } else {
        return getRegularComponent<C>(item, manager);
    }
}

inline Item makeItem(ItemType type, ItemManager& manager) {
    auto* prototype = getPrototype(type, manager);
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
    manager.components.addComponent(item.componentsLoc, C::ID);
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
    manager.components.addComponent(item.componentsLoc, C::ID);
    item.signature.set(C::ID);
}

inline ItemQuantity getStackSize(Item item, const ItemManager& manager) {
    return getPrototype(item.type, manager)->stackSize;
}

} // namespace items

using items::ItemManager;

#endif