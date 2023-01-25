#ifndef ITEMS_ITEMS_INCLUDED
#define ITEMS_ITEMS_INCLUDED

#include "Item.hpp"
#include "components.hpp"
#include "Tiles.hpp"
#include "manager.hpp"
#include "Inventory.hpp"
#include "Prototype.hpp"

namespace items {

ItemStack makeGrenadeStack(ItemManager& manager, ItemQuantity quantity = 1) {
    auto grenade = makeItem(ItemTypes::Grenade, manager);
    addComponent<ITC::Wetness>(grenade, manager, {10});
    addComponent(grenade, manager, ITC::Durability{10});
    ItemStack stack(grenade, quantity);
    return stack;
}
/*
template<ItemID itemID, typename... Components>
struct CustomItemStack : ItemStackBase, SECS::CustomEntity<Components...> {
    CustomItemStack(ItemManager& manager) : ItemStackBase(itemID), SECS::CustomEntity<Components...>(manager) {

    }

    CustomItemStack(ItemManager& manager, const Components& ...components)
    : ItemStackBase(itemID), SECS::CustomEntity(manager, components...) {

    }
};


namespace Items {
    using namespace ITC;

    template<ItemID itemID, typename... Components>
    struct Base : ItemStack {
        Base(ItemManager& manager) : ItemStack(itemID) {
            //this->addComponent(manager, )
        }
    };
    /
    struct GrenadeItem : CustomItemStack<ItemIDs::Grenade, Wetness, Durability> {
        GrenadeItem(ItemManager& manager) : CustomItemStack(manager) {
            
        }

        GrenadeItem(ItemManager& manager, ItemQuantity quantity = 1) : CustomItemStack(manager, Wetness{0}, Durability{100.0f}) {

        }
    };
    /

    struct Tile : Base<ItemIDs::Tile> {
        Tile(ItemManager& manager, TileType tileType) : Base(manager) {
            add<ITC::Placeable>(manager, {tileType});
        }
    };
}

template<typename T>
using ConstRef = const T&;

template<ItemID itemID, class... Components>
ItemStack maker(ItemManager& manager, const Components& ...components) {
    auto stack = ItemStack(itemID);
    FOR_EACH_VAR_TYPE(stack.add<Components>(manager, components));
    return stack;
}

ItemStack TileMaker(ItemManager& manager, TileType tileType) {
    /
    auto s = ItemStack(ItemIDs::Tile);
    s.addComponent<ITC::Placeable>(manager, {tileType});
    return s;
    /
    
    return maker<ItemTypes::Tile>(manager, ITC::Placeable{tileType});
}

struct ItemCreator {
    ItemManager* manager;

    template<class T, typename... Params>
    auto make(ItemQuantity quantity, Params ...params) {
        auto stack = T(*manager, params);
        stack.quantity = quantity;
        return stack;
    }

    template<typename... Params, class Functor>
    ItemStack make(Functor functor, Params ...params) {
        auto stack = functor(*manager, params);
        stack.quantity = quantity;
        return stack;
    }

    ItemStack grenade() {
        
    }
};
*/

namespace Prototypes {
    namespace IDs = ItemTypes;
    using namespace ITC::Proto;

    struct None : ItemPrototype {
        None(PrototypeManager* manager) : ItemPrototype(New(manager, IDs::None)) {

        }
    };

    struct SandGun : ItemPrototype {
        SandGun(PrototypeManager* manager) : ItemPrototype(New<>(manager, IDs::SandGun)) {
            this->stackSize = 1;
            this->inventoryIcon = TextureIDs::Tiles::Sand;
        }
    };

    /*
    template<class... Components>
    using Prototype = ItemPrototypeSpecialization<Components...>;

    struct GrenadePrototype : Prototype<ITPC::Wettable, ITPC::StartDurability> {
        GrenadePrototype() : Prototype<ITPC::Wettable, ITPC::StartDurability>() {

        }
    };
    */
}

namespace Items {
    Item sandGun(ItemManager& manager) {
        auto i = makeItem(ItemTypes::SandGun, manager);
        addComponent<ITC::Durability>(i, manager, {100});
        return i;
    }

    Item tile(ItemManager& manager, TileType type) {
        auto i = makeItem(ItemTypes::Tile, manager);
        addComponent<ITC::Tile>(i, manager, {type});
        return i;
    }
}



inline float getFuelValue(ItemManager& manager, ItemStack stack) {
    auto fuelComponent = getComponent<ITC::Fuel>(stack.item, manager);

    float itemFuelValue = fuelComponent ? fuelComponent->value : NAN;
    return stack.quantity * itemFuelValue;
}

void serializeItem(Item item, void* out) {
    
}

void deserializeItem(Item item, void* in) {
    
}

void serializeComponents() {
    
}

PrototypeManager* makePrototypes(ComponentInfoRef componentInfo) {
    auto manager = new PrototypeManager(componentInfo, ItemTypes::Count);
    namespace ids = ItemTypes;
    using namespace Prototypes;
    auto prototypeNone = Prototypes::None(manager);
    //manager->add(prototypeNone);
    //manager->add(Prototypes::Grenade(manager));

    // check if we forgot any
    for (ItemType type = ids::None+1; type < ItemTypes::Count; type++) {
        if (manager->prototypes[type].id == ids::None) {
            LogError("FAIL ON PROTOTYPE MAKING!");
            assert(false);
        }
    }
    return manager;
}

} // namespace items

#endif