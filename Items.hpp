#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include <SDL2/SDL.h>
#include "Log.hpp"

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
    ItemStack *items; // the items held in the inventory
    int size;

    Inventory() {
        size = 0;
        items = NULL;
    }
    Inventory(int size) {
        if (size < 0) {
            LogError("Inventory::Constructor : Size given (%d) is < 0! Using size 0.", size);
            size = 0;
        }
        this->size = size;
        items = new ItemStack[size];
    }

    void destroy() {
        if (items) {
            delete[] items;
            items = NULL;
        }
    }

    /*
    * Add a stack of items to the inventory, stacking with the first available slot.
    * If the inventory is full, the stack will not be added and the return value will be 0,
    * if the inventory is near full, only a partial amount of the stack may be added.
    * In order to not duplicate items, it's likely necessary to use the return value of this function
    * to subtract from the quantity of the stack passed in.
    * @return The number of items of the stack that were added to the inventory.
    */
    int addItemStack(ItemStack stack) {
        int itemsAdded = 0;
        // find first unused slot or first slot of same type and add it to that slot
        for (int i = 0; i < size; i++) {
            if (!items[i].item) {
                // completely open slot, just copy the whole stack in.
                items[i] = stack;
                itemsAdded += stack.quantity;
                // no more items left to add, break.
                break;
            }
            else if (items[i].item == stack.item) {
                int room = ItemData[items[i].item].stackSize - items[i].quantity;
                // if the slot is entirely full this will be <= 0
                // get the minimum of how much items the slot can fit and how many items are in the stack that needs to be added
                if (room > 0) {
                    int itemsToAdd = stack.quantity;
                    if (itemsToAdd > room) {
                        itemsToAdd = room;
                    }
                    // move as many items as can fit from the stack to the inventory
                    items[i].quantity += itemsToAdd;
                    stack.quantity -= itemsToAdd;
                    itemsAdded += itemsToAdd;
                    if (stack.quantity == 0) {
                        // could fit the remaining entire stack into one slot
                        return itemsAdded;
                    } else {
                        // else need to use another slot because there isn't enough room to fit all the items in the stack
                        continue;
                    }
                } else {
                    // no room in this slot, continue on
                }
            }
        }
        // couldnt fit atleast some of the stack in the inventory
        return itemsAdded;
    }


    /*
    * Remove a stack of items from the inventory, removing sequentially from the slots containing the item.
    * If the inventory is empty, the stack will not be removed and the return value will be 0,
    * if the inventory is near empty, only a partial amount of the stack may be added.
    * In order to not remove more items than should be possible, it's likely necessary to use the return value of this function.
    * @return The number of items of the stack that were removed from the inventory.
    */
    int removeItemStack(ItemStack stack) {
        int itemsRemoved = 0;
        // find first unused slot or first slot of same type and add it to that slot
        for (int i = 0; i < size; i++) {
            if (items[i].item == stack.item) {

                // get the minimum of the two quanities, to not remove more than the stack said to,
                // and not to remove more than are in the inventory slot
                int itemsToRemove = (stack.quantity < items[i].quantity) ? stack.quantity : items[i].quantity;
                stack.quantity -= itemsToRemove;
                itemsRemoved += itemsToRemove;
                // in theory the quantity should never be below 0, so it could just be ==, but use <= to be safe
                if (stack.quantity <= 0) {
                    break;
                }
            }
        }
        // couldnt fit atleast some of the stack in the inventory
        return itemsRemoved;
    }

    

    int itemCount(Item item) {
        int count = 0;
        for (int i = 0; i < size; i++) {
            if (items[i].item == item) {
                count += items[i].quantity;
            }
        }
        return count;
    }
};

/*
size_t inventorySize(size_t size) {
    if (size < 17) {
        return 16;
    }
    if (size < 33) {
        return 32;
    }
    if (size < 65) {
        return 64;
    } else {
        
    }

    return 0;
}
*/


#endif