#ifndef GAME_STATE_INCLUDED
#define GAME_STATE_INCLUDED

#include <functional>
#include "llvm/BitVector.h"
#include "constants.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Tiles.hpp"
#include "Chunks.hpp"
#include "world/EntityWorld.hpp"
#include "world/functions.hpp"
#include "Player.hpp"

struct GameState {
    ChunkMap chunkmap;
    EntityWorld* ecs;
    ECS::Systems::SystemManager* ecsSystems;
    Player player;
    ItemManager itemManager;
    ScratchAllocator<> scratch;

    void init(const TextureManager* textureManager);
    void createWorld();
    void destroy();
};

/*
* Get a line of the whole number x,y pairs on the path from start to end, using DDA.
*/
SmallVector<IVec2> raytraceDDA(Vec2 start, Vec2 end);

void worldLineAlgorithm(Vec2 start, Vec2 end, const std::function<int(IVec2)>& callback);

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Boxf bounds, const std::function<void(ChunkData*)>& callback);

constexpr Vec2 EntityMaxPos = {(float)INT_MAX, (float)INT_MAX};
constexpr Vec2 EntityMinPos = {(float)INT_MIN, (float)INT_MIN};

constexpr inline bool isValidEntityPosition(Vec2 p) {
    return p.x > EntityMinPos.x && p.y > EntityMinPos.y
        && p.x < EntityMaxPos.x && p.y < EntityMaxPos.y;
}

inline void setEntityStaticPosition(GameState* state, Entity entity, Vec2 position) {
    auto* positionEc = state->ecs->Get<World::EC::Position>(entity);
    Vec2 oldPos = positionEc->vec2();
    positionEc->x = position.x;
    positionEc->y = position.y;
    World::entityPositionChanged(state, entity, oldPos);
}

#endif