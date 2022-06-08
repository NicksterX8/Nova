#ifndef GAME_STATE_INCLUDED
#define GAME_STATE_INCLUDED

#include <unordered_map>
#include <functional>
#include <array>
#include <vector>

#include <SDL2/SDL.h>
#include "constants.hpp"
#include "Textures.hpp"
#include "NC/cpp-vectors.hpp"
#include "NC/utils.h"

#include "Tiles.hpp"
#include "Chunks.hpp"
#include "Player.hpp"
#include "Entities/ECS.hpp"
#include "GameViewport.hpp"

struct GameState {
    ChunkMap chunkmap;
    Player player;
    ECS ecs;

    void free();
    void init(SDL_Renderer*, GameViewport*);
};

/*
* Get a line of tile coordinates from start to end, using DDA.
* Returns a line of size lineSize that must be freed using free()
* @param lineSize a pointer to an int to be filled in the with size of the line.
*/
IVec2* worldLine(Vec2 start, Vec2 end, int* lineSize);

void worldLineAlgorithm(Vec2 start, Vec2 end, std::function<int(IVec2)> callback);

bool pointIsOnTileEntity(ECS* ecs, Entity tileEntity, IVec2 tilePosition, Vec2 point);

bool findTileEntityAtPosition(GameState* state, Vec2 position, Entity* foundEntity);

void removeEntityOnTile(ECS* ecs, Tile* tile);

void placeEntityOnTile(ECS* ecs, Tile* tile, Entity entity);

void useGrenade(GameState* state, Vec2 playerAim);

#endif