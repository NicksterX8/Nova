#pragma once

#include "items/Prototype.hpp"
#include "items/components/components.hpp"

namespace items {

namespace Prototypes {

namespace IDs = ItemTypes;
using namespace ITC::Proto;

struct Grenade : public items::ItemPrototype {
    Grenade(PrototypeManager* manager) : ItemPrototype(New<>(manager, ItemTypes::Grenade)) {
        this->stackSize = 64;
        ItemPrototype::set<ITC::Fuel>({100.0f, 20.0f});
        set<ITC::Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        this->set<ITC::Edible>({1.0f, 2.0f});
        
    }

    static Item make(ItemManager& manager) {
        Item i = Item(ItemTypes::Grenade);
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
        auto i = Item(ItemTypes::SandGun);
        addComponent<ITC::Display>(i, manager, {TextureIDs::Tiles::Sand});
        return i;
    }
};

} // namespace Prototypes

} // namespace items