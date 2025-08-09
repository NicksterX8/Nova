#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include "utils/ints.hpp"
#include <functional>
#include "ECS/Entity.hpp"
#include "My/Vec.hpp"
#include "ECS/ArchetypePool.hpp"
#include "ECS/ComponentInfo.hpp"
#include "ECS/componentMacros.hpp"

typedef Uint16 TextureID;

typedef Uint32 ItemType;
typedef Sint32 ItemQuantity;

namespace items {

struct IDGetter {
    template<class C>
    static constexpr Sint16 get() {
        return C::ID;
    }
};

#define MAX_ITEM_COMPONENTS 64

using ComponentID = ECS::ComponentID;

using ECS::ComponentInfoRef;
using ComponentSignature = ECS::Signature;

using ItemQuantity = Sint32;
constexpr ItemQuantity ItemQuantityInfinity = -1;

namespace ItemTypes {
    #define ITEM_TYPE_LIST None, Grenade, SandGun, Tile
    GEN_IDS(ItemTypes, Uint16, ITEM_TYPE_LIST, Count);  // 65k types should be way more than enough
}

using ECS::Entity;
constexpr Entity NullEntity = ECS::NullEntity;

struct Item : Entity {
    ItemType type;

    Item(ItemType type = ItemTypes::None, Entity entity = NullEntity) : Entity(entity), type(type) {}

    static Item None() {
        return Item();
    }
};


struct ItemStack {
    Item item;
    ItemQuantity quantity;

    ItemStack() : item(Item()), quantity(0) {}

    ItemStack(Item item) : item(item), quantity(1) {}

    ItemStack(Item item, ItemQuantity quantity) : item(item), quantity(quantity) {}

    static ItemStack None() {
        return {Item::None(), 0};
    }

    /*
    * Reduce the quantity of an item stack, removing the items.
    * @return The number of items removed.
    * This will be equal to the reduction parameter when the item stack quantity is >= reduction,
    * Otherwise is equal to the quantity of the item stack since you cannot remove more items than are there.
    */
    ItemQuantity reduceQuantity(ItemQuantity reduction);

    bool infinite() const {
        return quantity == ItemQuantityInfinity && item.type != ItemTypes::None;
    }

    bool empty() const {
        return item.type == ItemTypes::None || quantity == 0;
    }

    bool operator==(const ItemStack& stack) const {
        return item.type == stack.item.type 
            && item.id == stack.item.id
            && item.version == stack.item.version
            && quantity == stack.quantity;
    }
};

}

using items::Item;
using items::ItemStack;
namespace ItemTypes = items::ItemTypes;

#endif