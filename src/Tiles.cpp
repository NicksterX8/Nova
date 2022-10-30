#include "Tiles.hpp"
#include "constants.hpp"
#include <SDL2/SDL.h>
#include "Items.hpp"
#include "ECS/Entity.hpp"

float TilePixels = DEFAULT_TILE_PIXEL_SIZE;
float TileWidth = DEFAULT_TILE_PIXEL_SIZE;
float TileHeight = TileWidth * M_SQRT2;

TileTypeDataStruct TileTypeData[NUM_TILE_TYPES];

