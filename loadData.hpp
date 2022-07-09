#ifndef LOADDATA_INCLUDED
#define LOADDATA_INCLUDED

#include "Tiles.hpp"
#include "Items.hpp"

TileTypeDataStruct defaultTileTypeData = {
    .color = {0, 0, 0, 255},
    .background = NULL,
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
        SDL_WHITE,
        Textures.Tiles.error,
        Walkable
    };
    TileTypeData[Grass] = {
        {92, 156, 55, 255},
        Textures.Tiles.grass,
        Walkable
    };
    TileTypeData[Sand] = {
        {232, 230, 104, 255},
        Textures.Tiles.sand,
        Walkable
    };
    TileTypeData[Water] = {
        {15, 125, 210, 255},
        Textures.Tiles.water,
        0
    };
    TileTypeData[Space] = {
        {0, 0, 0, 255},
        NULL,
        0
    };
    TileTypeData[SpaceFloor] = {
        .color = {230, 230, 230, 255},
        .background = Textures.Tiles.spaceFloor,
        .flags = Walkable | Mineable,
        .mineable = {
            .item = Items::SpaceFloor
        }
    };
    TileTypeData[Wall] = {
        .color = {130, 130, 130, 255},
        .background = Textures.Tiles.wall,
        .flags = Mineable,
        .mineable = {
            .item = Items::Wall
        }
    };
}

ItemTypeData defaultItemData = {
    .icon = NULL,
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
        .icon = NULL,
        .flags = 0,
        .stackSize = 0
    };
    ItemData[SpaceFloor] = {
        .icon = Textures.Tiles.spaceFloor,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::SpaceFloor
        }
    };
    ItemData[Grass] = {
        .icon = Textures.Tiles.grass,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::Grass
        }
    };
    ItemData[Sand] = {
        .icon = Textures.Tiles.sand,
        .flags = Placeable,
        .stackSize = 64,
        .placeable = {
            .tile = TileTypes::Sand
        }
    };
    ItemData[Wall] = {
        .icon = Textures.Tiles.wall,
        .flags = Placeable,
        .stackSize = 32,
        .placeable = {
            .tile = TileTypes::Wall
        }
    };
    ItemData[Grenade] = {
        .icon = Textures.grenade,
        .flags = Usable,
        .stackSize = 64
    };
    ItemData[SandGun] = {
        .icon = Textures.Tiles.sand,
        .flags = Usable,
        .stackSize = 1,
        .usable = {
            .onUse = [](){
                Log("Hello from on use");
                return true;
            }
        }
    };
}

#endif