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
    int size = 0;
    ComponentManager* componentManager = NULL;
    constexpr static int InfiniteSize = -1;

    Inventory() : items(NULL), size(0) {

    }
    
    Inventory(InventoryAllocator& allocator, int size) : size(size) {
        assert(size >= 0 || size == InfiniteSize && "Inventory can not have negative size unless infinite");
        items = allocator.allocate(size);
    }

    void destroy(InventoryAllocator& allocator) {
        allocator.deallocate(items, size);
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
        assert(itemIndex >= 0 && itemIndex < this->size && "inventory item index out of bounds!");
        return items[itemIndex];
    }

    bool empty() const {
        return size == 0;
    }

    bool infiniteSize() const {
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

    /*
    * Remove a stack of items from the inventory, removing sequentially from the slots containing the item.
    * If the inventory is empty, the stack will not be removed and the return value will be 0,
    * if the inventory is near empty, only a partial amount of the stack may be added.
    * In order to not remove more items than should be possible, it's likely necessary to use the return value of this function.
    * @return The number of items of the stack that were removed from the inventory.
    */
    ItemQuantity removeItemStack(ItemStack stack);

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

    void destroy() {

    }
};

ItemQuantity addItemStackToInventory(ItemManager& itemManager, Inventory inventory) {

}

} // namespace items

using items::Inventory;

#endif