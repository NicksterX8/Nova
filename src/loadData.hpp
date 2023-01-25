#ifndef LOADDATA_INCLUDED
#define LOADDATA_INCLUDED

#include "Tiles.hpp"
#include "items/items.hpp"

void loadItemData() {
    /*
    using namespace ItemTypes;
    for (int i = 0; i < Count; i++) {
        ItemData[i] = defaultItemData;
    }

    ItemData[0] = {
        .icon = TextureIDs::Null,
        .flags = 0,
        .stackSize = 0
    };
    ItemData[SpaceFloor] = {
        .icon = TextureIDs::Tiles::SpaceFloor,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::SpaceFloor
        }
    };
    ItemData[Grass] = {
        .icon = TextureIDs::Tiles::Grass,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::Grass
        }
    };
    ItemData[Sand] = {
        .icon = TextureIDs::Tiles::Sand,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::Sand
        }
    };
    ItemData[Wall] = {
        .icon = TextureIDs::Tiles::Wall,
        .flags = Placeable,
        .stackSize = 32,
        .placeable = {
            .tile = TileTypes::Wall
        }
    };
    ItemData[Grenade] = {
        .icon = TextureIDs::Grenade,
        .flags = Usable,
        .stackSize = 64
    };
    ItemData[SandGun] = {
        .icon = TextureIDs::Tiles::Sand,
        .flags = Usable,
        .stackSize = 1,
        .usable = {
            .onUse = [](){
                LogInfo("Hello from on use");
                return true;
            }
        }
    };
    */
}

#endif