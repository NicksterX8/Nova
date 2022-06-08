#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include <functional>
#include <SDL2/SDL.h>
#include "Log.hpp"

typedef Uint16 ItemID;
typedef Uint32 ItemFlags;
typedef Uint8 Item;

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
        Edible = 2,
        Usable = 4
    };

    struct PlaceableItem {
        TileType tile; // a tile that is placed when this item is placed
    };

    struct EdibleItem {
        float hungerValue;
    };

    struct UseableItem {
        
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
};
extern ItemTypeData ItemData[NUM_ITEMS];

const unsigned int INFINITE_ITEM_QUANTITY = (unsigned int)(-1);

struct ItemStack {
    Item item;
    Uint32 quantity;

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

    ItemStack takeStack() {
        for (int i = 0; i < size; i++) {
            if (items[i].item) {
                ItemStack ret = items[i];
                items[i].item = 0;
                return ret;
            }
        }
        return ItemStack();
    }

    /*
    * Add a stack of items to the inventory, stacking with the first available slot.
    * If the inventory is full, the stack will not be added and the return value will be 0,
    * if the inventory is near full, only a partial amount of the stack may be added.
    * In order to not duplicate items, it's likely necessary to use the return value of this function
    * to subtract from the quantity of the stack passed in.
    * @return The number of items of the stack that were added to the inventory.
    */
    Uint32 addItemStack(ItemStack stack) {
        Uint32 itemsLeft = stack.quantity;
        // find first unused slot or first slot of same type and add it to that slot
        for (int i = 0; i < size && itemsLeft > 0; i++) {
            if (stack.quantity == INFINITE_ITEM_QUANTITY) {
                if (!items[i].item) {
                    items[i] = stack;
                    return INFINITE_ITEM_QUANTITY;
                }
            }
            // else need to use another slot
            else if (!items[i].item) {
                // add as many items as can fit based on the item's stack size and how many are in the stack
                // if it weren't for the possibility of a stack of items having a quantity higher
                // than the stack's stacksize it would work to just copy the stack straight into the inventory slot
                items[i].item = stack.item;
                Uint32 itemsToAdd = (ItemData[stack.item].stackSize < itemsLeft) ? 
                                    ItemData[stack.item].stackSize : itemsLeft;
                items[i].quantity = itemsToAdd;
                itemsLeft -= itemsToAdd;
            }
            else if (items[i].item == stack.item) {
                int room = ItemData[items[i].item].stackSize - items[i].quantity;
                // if the slot is entirely full this will be <= 0
                // get the minimum of how much items the slot can fit and how many items are in the stack that needs to be added
                if (room > 0) {
                    Uint32 itemsToAdd = (itemsLeft < (Uint32)room) ? itemsLeft : (Uint32)room;
                    // move as many items as can fit from the stack to the inventory
                    items[i].quantity += itemsToAdd;
                    itemsLeft -= itemsToAdd;
                    
                }
                // no room left in this slot, continue on
            }
        }
        // couldnt fit atleast some of the stack in the inventory
        return stack.quantity - itemsLeft;
    }


    /*
    * Remove a stack of items from the inventory, removing sequentially from the slots containing the item.
    * If the inventory is empty, the stack will not be removed and the return value will be 0,
    * if the inventory is near empty, only a partial amount of the stack may be added.
    * In order to not remove more items than should be possible, it's likely necessary to use the return value of this function.
    * @return The number of items of the stack that were removed from the inventory.
    */
    Uint32 removeItemStack(ItemStack stack) {
        Uint32 itemsLeft = stack.quantity;
        // go through slots of the same type as the stack and
        // remove as many items as possible from that slot
        for (int i = 0; i < size; i++) {
            if (items[i].item == stack.item) {
                // get the minimum of the two quanities, to not remove more than the stack said to,
                // and not to remove more than are in the slot
                Uint32 itemsToRemove = (itemsLeft < items[i].quantity) ? itemsLeft : items[i].quantity;
                itemsLeft -= itemsToRemove;
                if (itemsLeft == 0) {
                    break;
                }
            }
        }
        return stack.quantity - itemsLeft;
    }

    Uint32 itemCount(Item item) {
        Uint32 count = 0;
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