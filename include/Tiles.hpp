#ifndef TILES_INCLUDED
#define TILES_INCLUDED

#include "constants.hpp"
#include "rendering/textures.hpp"
#include <SDL3/SDL.h>
#include "items/items.hpp"

namespace TileTypes {
    enum E: Uint16 {
        Empty = 0,
        Space = 1,
        Grass,
        Sand,
        Water,
        SpaceFloor,
        GreyFloor,
        GreyFloorOverhang,
        Wall,
        TransportBelt,
        Count
    };

    enum Flags: Uint32 {
        Solid = 1, // cant be walked on 
        Mineable = 2
    };

    struct MineableTile {
        ItemStack item; // item given by being mined (or 0 for nothing)
    };
}
typedef Uint16 TileType;
typedef TileTypes::Flags TileTypeFlag;

#define NUM_TILE_TYPES TileTypes::Count

struct TileTypeDataStruct {
    SDL_Color color = {255, 0, 0, 255};
    TextureID background = 0;
    Animation animation = {
        .texture = 0,
        .frameSize = {-1, -1},
        .frameCount = 0,
        .updatesPerFrame = 1
    };
    Uint32 flags = 0;

    TileTypes::MineableTile mineable;
};

extern TileTypeDataStruct TileTypeData[NUM_TILE_TYPES];

struct Tile {
    TileType type;

    Tile() : type(TileTypes::Empty) {

    }
    
    Tile(TileType type): type(type) {

    }
};

namespace items {
    struct ItemManager;
}

void loadTileData(items::ItemManager& itemManager, const TextureManager* textureManager);

#endif