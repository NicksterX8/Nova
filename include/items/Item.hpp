#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include "utils/ints.hpp"
#include <functional>
#include "ECS/secsy.hpp"
#include "My/Vec.hpp"
#include "ECS/generic/components.hpp"
#include "ECS/generic/ecs.hpp"

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

using ItemECS = PECS::EcsClass;
using ComponentID = GECS::ComponentID;

using ComponentPtr = GECS::ComponentPtr;

using ComponentsAddress = GECS::ArchetypeElementAddress;
constexpr auto NullComponentsAddress = GECS::NullElementAddress;

using GECS::ComponentInfoRef;
using ComponentSignature = ItemECS::Signature;

using ItemQuantity = Sint32;
constexpr ItemQuantity ItemQuantityInfinity = -1;

namespace ItemTypes {
    #define ITEM_TYPE_LIST None, Grenade, SandGun, Tile
    GEN_IDS(ItemTypes, Uint16, ITEM_TYPE_LIST, Count);  // 65k types should be way more than enough
}

struct Item : PECS::Entity {
 
    Item() : PECS::Entity(ItemTypes::None, 0, NullComponentsAddress) {};

    Item(PECS::Entity entity) : PECS::Entity(entity) {}

    Item(ItemType type, ComponentSignature signature = 0, ComponentsAddress components = NullComponentsAddress) : PECS::Entity(type, signature, components) {}

    static Item None() {
        return {ItemTypes::None, ComponentSignature{0}, NullComponentsAddress};
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
};

}

using items::Item;
using items::ItemStack;
namespace ItemTypes = items::ItemTypes;

#endif