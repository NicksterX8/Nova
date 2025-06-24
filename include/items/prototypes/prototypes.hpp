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

struct Tile : ItemPrototype {
    Tile(PrototypeManager& manager) : ItemPrototype(manager.New(ItemTypes::Tile)) {
        setName("tile");
        add<ITC::StackSize>({64});
    }

    static Item make(ItemManager& m, TileType tile) {
        Item i = m.newItem(ItemTypes::Tile);
        m.addComponent<ITC::Placeable>(i, {tile});
        m.addComponent<ITC::Display>(i, ITC::Display{TileTypeData[tile].background});
        
        return i;
    }
};

struct Grenade : items::ItemPrototype {
    Grenade(PrototypeManager& manager) : ItemPrototype(manager.New(ItemTypes::Grenade)) {
        setName("grenade");
        add<ITC::StackSize>({64});
        add<ITC::Fuel>({100.0f, 20.0f});
        add<ITC::Edible>({.hungerValue = 1.0f, .saturation = 2.0f});
        add<ITC::Usable>({onUse, true});
        
    }

    static bool onUse(Game* g);

    static Item make(ItemManager& manager) {
        Item i = manager.newItem(ItemTypes::Grenade);
        manager.addComponent<ITC::Display>(i, {TextureIDs::Grenade});
        return i;
    }
};

struct None : ItemPrototype {
    None(PrototypeManager& manager) : ItemPrototype(manager.New(IDs::None)) {

    }
};

struct SandGun : ItemPrototype {
    SandGun(PrototypeManager& manager) : ItemPrototype(manager.New(IDs::SandGun)) {
        setName("sand-gun");
        add<ITC::StackSize>({1});
        add<ITC::Usable>({onUse, false});
    }

    static Item make(ItemManager& manager) {
        auto i = manager.newItem(ItemTypes::SandGun);
        manager.addComponent<ITC::Display>(i, {TextureIDs::Tiles::Sand});
        return i;
    }

    static bool onUse(Game* g);
};

} // namespace Prototypes

} // namespace items