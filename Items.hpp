#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include <SDL2/SDL.h>
#include <functional>

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
* An inventory storing a number of item stacks
*/
struct Inventory {
    ItemStack *items; // the items held in the inventory
    Uint32 size;

    Inventory() {
        size = 0;
        items = NULL;
    }
    Inventory(Uint32 size) {
        this->size = size;
        // TODO: Use a better method of item stack allocation
        items = new ItemStack[size];
    }

    void destroy() {
        if (items) {
            // TODO: change this 
            delete[] items;
            items = NULL;
        }
    }

    /*
    * Get the first available item stack in the inventory,
    * skipping empty item stacks
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
};

#endif