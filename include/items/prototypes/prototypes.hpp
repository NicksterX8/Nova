#pragma once

#include "items/Prototype.hpp"
#include "items/components/components.hpp"
#include "items/manager.hpp"

struct Game;

namespace items {

namespace Prototypes {

namespace IDs = ItemTypes;
using namespace ITC::Proto;

struct Grenade : public items::ItemPrototype {
    Grenade(PrototypeManager* manager) : ItemPrototype(New<>(manager, ItemTypes::Grenade)) {
        this->stackSize = 64;
        add<ITC::Fuel>({100.0f, 20.0f});
        add<ITC::Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        add<ITC::Usable>({onUse});
        
    }

    static bool onUse(Game* g);

    static Item make(ItemManager& manager) {
        Item i = makeItem(ItemTypes::Grenade, manager);
        addComponent<ITC::Display>(i, manager, {TextureIDs::Grenade});
        return i;
    }
};

struct None : ItemPrototype {
    None(PrototypeManager* manager) : ItemPrototype(New(manager, IDs::None)) {

    }
};

struct SandGun : ItemPrototype {
    SandGun(PrototypeManager* manager) : ItemPrototype(New<>(manager, IDs::SandGun)) {
        this->stackSize = 1;
    }

    static Item make(ItemManager& manager) {
        auto i = makeItem(ItemTypes::SandGun, manager);
        addComponent<ITC::Display>(i, manager, {TextureIDs::Tiles::Sand});
        return i;
    }
};

} // namespace Prototypes

} // namespace items