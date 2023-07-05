#include "Tiles.hpp"
#include "constants.hpp"
#include <SDL2/SDL.h>
#include "items/items.hpp"
#include "ECS/Entity.hpp"
#include "items/manager.hpp"

TileTypeDataStruct TileTypeData[NUM_TILE_TYPES];

template<typename... Flags>
static Uint32 flagsSignature(Flags... flagList) {
    std::array<TileTypeFlag, sizeof...(Flags)> values = {flagList ...}; // std array
    Uint32 flags = 0;
    for (auto flag : values) {
        flags |= flag;
    }
    return flags;
}

static const TileTypeDataStruct defaultTileTypeData = {
    .color = {0, 0, 0, 255},
    .background = TextureIDs::Null,
    .flags = 0,
    .mineable = {
        .item = ItemStack::None()
    }
};

#define NewTile(type) assert(type < NUM_TILE_TYPES && "Invalid tile type; type out of range"); tile = &Data[type]; *tile = defaultTileTypeData;
#define Color(...) tile->color = __VA_ARGS__;
#define Background(bg) tile->background = bg;
#define Flags(...) tile->flags = flagsSignature(__VA_ARGS__);

void loadTileData(items::ItemManager& itemManager) {
    using namespace TileTypes;
    TileTypeDataStruct* Data = TileTypeData;
    for (int i = 0; i < NUM_TILE_TYPES; i++) {
        Data[i] = defaultTileTypeData;
    }

    TileTypeDataStruct* tile = nullptr;
    
    NewTile(Empty)
        Color({255, 255, 255, 255})
        Background(TextureIDs::Null)
        Flags(Walkable)

    NewTile(Grass)
        Color({92, 156, 55, 255})
        Background(TextureIDs::Tiles::Grass)
        Flags(Walkable)

    NewTile(Sand)
        Color({232, 230, 104, 255})
        Background(TextureIDs::Tiles::Sand)
        Flags(Walkable)
    
    NewTile(Water)
        Color({15, 125, 210, 255})
        Background(TextureIDs::Tiles::Water)
        Flags()

    NewTile(Space)
        Color({0, 0, 0, 255})
        Background(TextureIDs::Null)
        Flags()

    NewTile(SpaceFloor)
        Color({230, 230, 230, 255})
        Background(TextureIDs::Tiles::SpaceFloor)
        Flags(Walkable, Mineable)
        tile->mineable.item = items::Items::tile(itemManager, TileTypes::SpaceFloor);

     NewTile(Wall)
        Color({130, 130, 130, 255})
        Background(TextureIDs::Tiles::Wall)
        Flags(Mineable)
        tile->mineable.item = items::Items::tile(itemManager, TileTypes::Wall);
}

