#ifndef LOADDATA_INCLUDED
#define LOADDATA_INCLUDED

#include "Tiles.hpp"
#include "Items.hpp"

TileTypeDataStruct defaultTileTypeData = {
    .color = {0, 0, 0, 255},
    .background = TextureIDs::Null,
    .flags = 0,
    .mineable = {
        .item = 0
    }
};

void loadTileData() {
    using namespace TileTypes;
    TileTypeDataStruct* Data = TileTypeData;
    for (int i = 0; i < NUM_TILE_TYPES; i++) {
        Data[i] = defaultTileTypeData;
    }

    Data[Empty] = {
        {255, 255, 255, 255},
        TextureIDs::Null,
        Walkable
    };
    TileTypeData[Grass] = {
        {92, 156, 55, 255},
        TextureIDs::Tiles::Grass,
        Walkable
    };
    TileTypeData[Sand] = {
        {232, 230, 104, 255},
        TextureIDs::Tiles::Sand,
        Walkable
    };
    TileTypeData[Water] = {
        {15, 125, 210, 255},
        TextureIDs::Tiles::Water,
        0
    };
    TileTypeData[Space] = {
        {0, 0, 0, 255},
        TextureIDs::Null,
        0
    };
    TileTypeData[SpaceFloor] = {
        .color = {230, 230, 230, 255},
        .background = TextureIDs::Tiles::SpaceFloor,
        .flags = Walkable | Mineable,
        .mineable = {
            .item = Items::SpaceFloor
        }
    };
    TileTypeData[Wall] = {
        .color = {130, 130, 130, 255},
        .background = TextureIDs::Tiles::Wall,
        .flags = Mineable,
        .mineable = {
            .item = Items::Wall
        }
    };
}

ItemTypeData defaultItemData = {
    .icon = TextureIDs::Null,
    .flags = 0,
    .stackSize = 1,
    .placeable = {
        .tile = 0
    }
};

void loadItemData() {
    using namespace Items;
    for (int i = 0; i < NUM_ITEMS; i++) {
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
}

#endif