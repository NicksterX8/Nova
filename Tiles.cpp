#include "Tiles.hpp"
#include "constants.hpp"
#include "NC/colors.h"
#include "Textures.hpp"
#include <SDL2/SDL.h>
#include "Items.hpp"
#include "Entities/Entity.hpp"

float TilePixels = DEFAULT_TILE_PIXEL_SIZE;

TileTypeDataStruct TileTypeData[NUM_TILE_TYPES];

Tile::Tile() {
    type = TileTypes::Empty;
    entity.id = NULL_ENTITY;
}
Tile::Tile(TileType type): type(type) {
    entity.id = NULL_ENTITY;
}