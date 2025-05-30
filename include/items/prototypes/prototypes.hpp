#pragma once

#include "items/components/components.hpp"
#include "items/manager.hpp"
#include "rendering/textures.hpp"
#include "Tiles.hpp"

struct Game;

namespace items {

namespace Prototypes {

namespace IDs = ItemTypes;
using namespace ITC::Proto;
using PECS::New;

struct Tile : public ItemPrototype {
    Tile(PrototypeManager* manager) : ItemPrototype(New<>(manager, ItemTypes::Tile)) {
        add<ITC::StackSize>({64});
    }

    static Item make(ItemManager& m, TileType tile) {
        Item i = makeItem(ItemTypes::Tile, m);
        addComponent<ITC::Placeable>(i, m, {tile});
        addComponent<ITC::Display>(i, m, ITC::Display{TileTypeData[tile].background});
        return i;
    }
};

struct Grenade : public items::ItemPrototype {
    Grenade(PrototypeManager* manager) : ItemPrototype(New<>(manager, ItemTypes::Grenade)) {
        add<ITC::StackSize>({64});
        add<ITC::Fuel>({100.0f, 20.0f});
        add<ITC::Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        add<ITC::Usable>({onUse, true});
        
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
        add<ITC::StackSize>({64});
        add<ITC::Usable>({onUse, false});
    }

    static Item make(ItemManager& manager) {
        auto i = makeItem(ItemTypes::SandGun, manager);
        addComponent<ITC::Display>(i, manager, {TextureIDs::Tiles::Sand});
        return i;
    }

    static bool onUse(Game* g);
};

} // namespace Prototypes

} // namespace items