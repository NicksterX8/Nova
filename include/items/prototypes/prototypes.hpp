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
        this->inventoryIcon = TextureIDs::Grenade;
        ItemPrototype::set<ITC::Fuel>({100.0f, 20.0f});
        set<ITC::Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        this->set<ITC::Edible>({1.0f, 2.0f});
        
    }
};

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

} // namespace Prototypes

} // namespace items