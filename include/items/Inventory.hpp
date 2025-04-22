#ifndef ITEMS_INVENTORY_INCLUDED
#define ITEMS_INVENTORY_INCLUDED

#include "Item.hpp"
#include "manager.hpp"

struct Header {
    int size;
    int* data;

    int get(int s) {
        return data[s];
    }
};

struct SmallArray : Header {
    
};

namespace items {

template<size_t N>
struct SizedInventory {
    std::array<ItemStack, N> items;

    int size() const {
        return (int)items.size();
    }
};

/*
* An inventory storing a number of item stacks
*/
struct Inventory {
    using SizeT = int;
    ItemStack* items = NULL;
    ItemQuantity size = 0;
    //Uint16 startIndex = 0; // The index of the first non-empty item slot, or 0 if the inventory is empty
    //Uint16 endIndex = 0; // the index one past the index of the last non-empty item slot
    ItemManager* manager = NULL;
    int refCount = 0;
    constexpr static int InfiniteSize = -1;

    Inventory() = default;
    
    Inventory(ItemManager* manager, int size) : size(size), manager(manager) {
        assert(size >= 0 || size == InfiniteSize && "Inventory can not have negative size unless infinite");
        items = manager->inventoryAllocator.allocate(size);
        for (int i = 0; i < size; i++) {
            items[i] = ItemStack::None();
        }
    }

    void addRef() {
        refCount++;
    }
    
    void removeRef() {
        if (--refCount <= 0) {
            this->destroy();
        }
    }

    void destroy() {
        manager->inventoryAllocator.deallocate(items, size);
        size = 0;
        items = NULL;
    }

    ItemType& type(int slot) const {
        return items[slot].item.type;
    }

    ItemQuantity& quantity(int slot) const {
        return items[slot].quantity;
    }

    ComponentsAddress& componentsLoc(int slot) const {
        return items[slot].item.componentsLoc;
    }

    ComponentSignature& signature(int slot) const {
        return items[slot].item.signature;
    }

    inline ItemStack& get(Sint32 itemIndex) {
        assert(itemIndex >= 0 && itemIndex < this->size && "inventory item index out of bounds!");
        return items[itemIndex];
    }

    inline const ItemStack& get(Sint32 itemIndex) const {
        //assert(itemIndex >= this->startIndex && itemIndex < this->endIndex && "inventory item index out of bounds! The start and end indices may be incorrect.");
        assert(itemIndex < size && itemIndex >= 0);
        return items[itemIndex];
    }

    bool empty() const {
        //return endIndex == 0;
        return false;
    }

    bool isInfinite() const {
        return size == InfiniteSize;
    }

    /*
    * Get the first available item stack in the inventory, skipping empty item stacks,
    * and remove it from the inventory.
    */
    ItemStack takeFirstItemStack();

    /*
    * Add a stack of items to the inventory, stacking with the first available slot.
    * If the inventory is full, the stack will not be added and the return value will be 0,
    * if the inventory is near full, only a partial amount of the stack may be added.
    * In order to not duplicate items, it's likely necessary to use the return value of this function
    * to subtract from the quantity of the stack passed in.
    * @return The number of items of the stack that were added to the inventory.
    */
    ItemQuantity addItemStack(ItemStack stack);

    ItemQuantity removeItemType(ItemType type, ItemQuantity quantity);

    /*
    * Get the number of items of the given type in the inventory,
    * summing all the quantities of the stacks of the type in the inventory.
    */
    ItemQuantity itemCount(ItemType type);

    inline ItemStack& operator[](Uint32 index) {
        return get(index);
    }

    inline const ItemStack& operator[](Uint32 index) const {
        return get(index);
    }
};

} // namespace items

using items::Inventory;

#endif