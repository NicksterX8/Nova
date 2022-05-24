#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include <SDL2/SDL.h>

typedef Uint16 ItemID;
typedef Uint32 ItemFlags;
typedef Uint16 TileType;
typedef Uint8 Item;

// WARNING: KEEP THIS UPDATED!!!
#define NUM_ITEMS 5 // Add one for the "none" item
namespace Items {
    enum IDs {
        SpaceFloor = 1,
        Grass,
        Sand,
        Wall
    };

    enum Flags {
        Placeable = 1,
        Edible = 2
    };

    struct PlaceableItem {
        TileType tile; // a tile that is placed when this item is placed
    };

    struct EdibleItem {
        float hungerValue;
    };
}

struct ItemTypeData {
    SDL_Texture* icon;
    ItemFlags flags;
    int stackSize;

    // flag data
    Items::PlaceableItem placeable;
    Items::EdibleItem edible;
};
extern ItemTypeData ItemData[NUM_ITEMS];

struct ItemStack {
    Item item;
    int quantity;

    ItemStack();
    // construct an item stack with a quantity of 1
    ItemStack(Item item);

    ItemStack(Item item, int quantity);

    /*
    * Reduce the quantity of an item stack, removing the items.
    * @return The number of items removed.
    * This will be equal to the reduction parameter when the item stack quantity is >= reduction,
    * Otherwise is equal to the quantity of the item stack since you cannot remove more items than are there.
    */
    int reduceQuantity(int reduction);
};

#define MAX_INVENTORY_SIZE 64

/*
* An inventory storing a number of item stacks
*/
struct Inventory {
    // just use an array of the max size instead of resizing it when needed
    ItemStack items[MAX_INVENTORY_SIZE]; // the items held in the inventory
    int size = 27; // the size of the inventory / number of slots

    Inventory() {}
    Inventory(int inventorySize);

    /*
    * Add a stack of items to the inventory, stacking with the first available slot.
    * If the inventory is full, the stack will not be added and the return value would be 0,
    * if the inventory is near full, only a partial amount of the stack may be added.
    * In order to not duplicate items, it's likely necessary to use the return value of this function
    * to subtract from the quantity of the stack passed in.
    * @return The number of items of the stack that were added to the inventory.
    */
    int addItemStack(ItemStack stack);
};

#endif