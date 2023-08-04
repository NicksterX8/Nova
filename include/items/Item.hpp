#ifndef ITEMS_INCLUDED
#define ITEMS_INCLUDED

#include "utils/ints.hpp"
#include <functional>
#include "ECS/secsy.hpp"
#include "My/Vec.hpp"
#include "ECS/generic/components.hpp"

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

using ItemECS = GECS::Class<MAX_ITEM_COMPONENTS, IDGetter>;
constexpr ItemECS ItemEcs;
using ComponentID = GECS::ComponentID;
using ComponentSignature = ItemECS::Signature; // 64 components or flags is probably good

// wrapper
template<class ...Cs>
constexpr ComponentSignature getComponentSignature() {
    return ItemECS::getSignature<Cs...>();
}
using ComponentPtr = GECS::ComponentPtr;

using ComponentsAddress = GECS::ArchetypeElementAddress;
constexpr auto NullComponentsAddress = GECS::NullElementAddress;

using GECS::ComponentInfo;
using GECS::ComponentInfoRef;

using ItemQuantity = Sint32;
constexpr ItemQuantity ItemQuantityInfinity = -1;

namespace ItemTypes {
    #define ITEM_TYPE_LIST None, Grenade, SandGun, Tile
    GEN_IDS(ItemTypes, Uint16, ITEM_TYPE_LIST, Count);  // 65k types should be way more than enough
}

using ItemType = Uint16;

struct Item {
    ComponentSignature signature;
    ComponentsAddress componentsLoc;
    ItemType type;
 
    Item() : signature(0), componentsLoc(NullComponentsAddress), type(ItemTypes::None) {};

    Item(ItemType type, ComponentSignature signature = 0, ComponentsAddress components = NullComponentsAddress) : signature(signature), componentsLoc(components), type(type) {}

    static Item None() {
        return {ItemTypes::None, ComponentSignature{0}, NullComponentsAddress};
    }

    template<class ...Cs>
    bool has() const {
        return has(getComponentSignature<Cs...>());
    }

    bool has(ComponentSignature components) const {
        return signature.hasAll(components);
    }

    bool has(ComponentID component) const {
        return signature[component];
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

/*
struct ItemStack {
    SECS::ComponentListC<64>* components = nullptr; // null if no components exist
    ItemID id = ItemIDs::None;
    ItemQuantity quantity = 0;

    ItemStack() = default;

    ItemStack(ItemID id) : id(id), quantity(1) {}

    ItemStack(ItemID id, ItemQuantity quantity) : id(id), quantity(quantity) {}

    template<typename C>
    void addComponent(ItemManager& manager, const C& value) {
        if (!components) {
            components = new SECS::ComponentList<64>();
        }
        auto storage = manager->allocateComponent<C>();
        *storage = value;
        components->add<C>(storage);
    }
};
*/

/*
struct ItemStack : ItemStackBase, protected SECS::Entity {
    ItemStack() = default;
    ItemStack(ItemStackBase base) : ItemStackBase(base), SECS::Entity() {}

    void clear() {
       // SECS::Entity::destroy();
        id = ItemIDs::None;
        quantity = 0;
    }
};*/

namespace ItemPrototypes {
/*
using namespace ProtoITC;
struct GrenadePrototype : ItemPrototypeMaker<ItemIDs::Grenade, ITC::Health, ITC::Durability> {
    GrenadePrototype(ItemPrototypeManager& manager): ItemPrototypeMaker(manager,
    ITC::Health{100.0f},
    ITC::Durability{1.0f}) {
        *get<ITC::Wetness>() = {0}; // error, not added
        add<ITC::Wetness>(0); // correct
        
    }
};
*/

}

 /*
    enum Flags : ItemFlags {
        Placeable = 1 << 0,
        Edible    = 1 << 1,
        Usable    = 1 << 2,
        Breakable = 1 << 3,
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
    TextureID icon;
    ItemFlags flags;
    Uint32 stackSize;

    // flag data
    ItemIDs::PlaceableItem placeable;
    ItemIDs::EdibleItem edible;
    ItemIDs::UseableItem usable;
    ItemIDs::BreakableItem breakable;
};
//extern ItemTypeData ItemData[NUM_ITEMS];


using ItemComponentID = SECS::ComponentID;

// get item component id
template<class C>
constexpr ID_Type getItcID() {
    return ITC_IDs::getID<C>();
}

template<class... Components>
constexpr ComponentFlags componentSignature() {
    constexpr ComponentID ids[] = {getID<Components>() ...};
    // sum component signatures
    ComponentFlags result(0);
    for (size_t i = 0; i < sizeof...(Components); i++) {
        if (!ECS_BAD_COMPONENT_ID(ids[i]))
            result.set((Uint32)ids[i]);
    }
    return result;
}*/

/*
inline void tester4() {
    My::DenseSparseSet<int, int> map;
    assert(!map.lookup(3));
    map.insert(2, 5);
    assert(*map.lookup(2) == 5);
    assert(map.remove(2));
    assert(!map.remove(2));
}
*/

}

using items::Item;
using items::ItemStack;
namespace ItemTypes = items::ItemTypes;

#endif