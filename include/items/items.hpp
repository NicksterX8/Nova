#ifndef ITEMS_ITEMS_INCLUDED
#define ITEMS_ITEMS_INCLUDED

#include "Item.hpp"
#include "components/components.hpp"
#include "Tiles.hpp"
#include "manager.hpp"
#include "Inventory.hpp"
#include "ECS/PrototypeManager.hpp"

namespace items {

inline ItemStack makeGrenadeStack(ItemManager& manager, ItemQuantity quantity = 1) {
    auto grenade = manager.newItem(ItemTypes::Grenade);
    manager.addComponent<ITC::Wetness>(grenade, {10});
    manager.addComponent(grenade, ITC::Durability{10});
    ItemStack stack(grenade, quantity);
    return stack;
}

namespace Items {
    inline Item sandGun(ItemManager& manager) {
        auto i = manager.newItem(ItemTypes::SandGun);
        manager.addComponent<ITC::Durability>(i, {100});
        manager.addComponent<ITC::Display>(i, ITC::Display{TextureIDs::Tiles::Sand});
        return i;
    }
}

PrototypeManager* makePrototypes(ECS::ComponentInfoRef componentInfo);

} // namespace items

#endif