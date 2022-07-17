#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include <SDL2/SDL.h>
#include <functional>
#include <unordered_map>
#include <vector>

typedef Uint16 Item;
typedef Uint32 ItemFlags;

// forward declare this
typedef Uint16 TileType;

// WARNING: KEEP THIS UPDATED!!!
#define NUM_ITEMS 7 // Add one for the "none" item
namespace Items {
    enum IDs {
        SpaceFloor = 1,
        Grass,
        Sand,
        Wall,
        Grenade,
        SandGun
    };

    enum Flags {
        Placeable = 1,
        Edible    = 2,
        Usable    = 4,
        Breakable = 8,
    };

    struct PlaceableItem {
        TileType tile; // a tile that is placed when this item is placed
    };

    struct EdibleItem {
        float hungerValue;
    };

    struct UseableItem {
        std::function<bool()> onUse;
    };

    struct BreakableItem {
        float durabilityPerUse;
    };
}

struct ItemTypeData {
    SDL_Texture* icon;
    ItemFlags flags;
    Uint32 stackSize;

    // flag data
    Items::PlaceableItem placeable;
    Items::EdibleItem edible;
    Items::UseableItem usable;
    Items::BreakableItem breakable;
};
extern ItemTypeData ItemData[NUM_ITEMS];

constexpr Uint32 INFINITE_ITEM_QUANTITY = (Uint32)(-1);

struct ItemStack {
    Item item;
    Uint32 quantity;
    float durability = 100.0f;

    ItemStack();
    // construct an item stack with a quantity of 1
    ItemStack(Item item);

    ItemStack(Item item, Uint32 quantity);

    /*
    * Reduce the quantity of an item stack, removing the items.
    * @return The number of items removed.
    * This will be equal to the reduction parameter when the item stack quantity is >= reduction,
    * Otherwise is equal to the quantity of the item stack since you cannot remove more items than are there.
    */
    int reduceQuantity(Uint32 reduction);
};

/*
class ItemPool {
public:
    using Address = Sint32;
    constexpr static Address NullAddress = -1;
private:

    ItemStack* buffer;
    Uint32 used;
    Uint32 reserved;
    std::unordered_map<Address, Uint32> map;
    std::vector<Address> freeAddresses;
    std::vector<Uint32> freeIndices;
public:
    ItemStack* get(Address address) const {
        auto it = map.find(address);
        if (it != map.end()) {
            return &buffer[it->second];
        }        

        return NULL;
    }


    Address alloc(Uint32 size) {
        if (size == INFINITE_ITEM_QUANTITY) {
            return NullAddress;
        }

        if (size + used > reserved) {
            resize((size+used*2));
        }

        Uint32 index = freeIndices.back();
        Address address = freeAddresses.back();
        freeIndices.pop_back();
        freeAddresses.pop_back();
        used += size;

        return address;
    }

    void deleteArray(Address address) {

    }

    int resize(Uint32 newSize) {
        
    }
};
*/

/*
* An inventory storing a number of item stacks
*/
struct Inventory {
    ItemStack* items;
    Uint32 size;

    Inventory() {
        size = 0;
        items = NULL;
    }
    
    Inventory(Uint32 size) {
        assert(size != INFINITE_ITEM_QUANTITY && "Inventory can not be infinitely large");
        items = new ItemStack[size];
        this->size = size;
    }

    void destroy() {
        delete items;
        size = 0;
        items = NULL;

    }

    inline ItemStack& get(Uint32 itemIndex) {
        return items[itemIndex];
    }

    inline const ItemStack& get(Uint32 itemIndex) const {
        return items[itemIndex];
    }

    /*
    * Get the first available item stack in the inventory,
    * skipping empty item stacks.
    */
    ItemStack firstItemStack();

    /*
    * Add a stack of items to the inventory, stacking with the first available slot.
    * If the inventory is full, the stack will not be added and the return value will be 0,
    * if the inventory is near full, only a partial amount of the stack may be added.
    * In order to not duplicate items, it's likely necessary to use the return value of this function
    * to subtract from the quantity of the stack passed in.
    * @return The number of items of the stack that were added to the inventory.
    */
    Uint32 addItemStack(ItemStack stack);


    /*
    * Remove a stack of items from the inventory, removing sequentially from the slots containing the item.
    * If the inventory is empty, the stack will not be removed and the return value will be 0,
    * if the inventory is near empty, only a partial amount of the stack may be added.
    * In order to not remove more items than should be possible, it's likely necessary to use the return value of this function.
    * @return The number of items of the stack that were removed from the inventory.
    */
    Uint32 removeItemStack(ItemStack stack);

    /*
    * Get the number of items of the given type in the inventory,
    * summing all the quantities of the stacks of the type in the inventory.
    */
    Uint32 itemCount(Item item);

    inline ItemStack& operator[](Uint32 index) {
        return get(index);
    }

    inline const ItemStack& operator[](Uint32 index) const {
        return get(index);
    }
};

#endif