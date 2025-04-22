#ifndef ITEMS_ITEMS_INCLUDED
#define ITEMS_ITEMS_INCLUDED

#include "Item.hpp"
#include "components/components.hpp"
#include "Tiles.hpp"
#include "manager.hpp"
#include "Inventory.hpp"
#include "Prototype.hpp"
#include "prototypes/prototypes.hpp"

namespace items {

inline ItemStack makeGrenadeStack(ItemManager& manager, ItemQuantity quantity = 1) {
    auto grenade = makeItem(ItemTypes::Grenade, manager);
    addComponent<ITC::Wetness>(grenade, manager, {10});
    addComponent(grenade, manager, ITC::Durability{10});
    ItemStack stack(grenade, quantity);
    return stack;
}

/*

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

namespace Items {
    inline Item sandGun(ItemManager& manager) {
        auto i = makeItem(ItemTypes::SandGun, manager);
        addComponent<ITC::Durability>(i, manager, {100});
        addComponent<ITC::Display>(i, manager, ITC::Display{TextureIDs::Tiles::Sand});
        return i;
    }

    Item tile(ItemManager& manager, TileType type);
}

inline float getFuelValue(ItemManager& manager, ItemStack stack) {
    auto fuelComponent = getComponent<ITC::Fuel>(stack.item, manager);

    float itemFuelValue = fuelComponent ? fuelComponent->value : NAN;
    return stack.quantity * itemFuelValue;
}

PrototypeManager* makePrototypes(ComponentInfoRef componentInfo);

} // namespace items

#endif