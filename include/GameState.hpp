#ifndef GAME_STATE_INCLUDED
#define GAME_STATE_INCLUDED

#include <functional>

#include "constants.hpp"
#include "utils/vectors.hpp"

#include "Tiles.hpp"
#include "Chunks.hpp"
#include "ECS/EntityWorld.hpp"
#include "Player.hpp"

struct GameState {
    ChunkMap chunkmap;
    EntityWorld ecs;
    Player player;
    ItemManager itemManager;

    void init();
    void destroy();
};

/*
* Get a line of the whole number x,y pairs on the path from start to end, using DDA.
*/
llvm::SmallVector<IVec2> raytraceDDA(Vec2 start, Vec2 end);

void worldLineAlgorithm(Vec2 start, Vec2 end, const std::function<int(IVec2)>& callback);

bool pointIsOnTileEntity(const EntityWorld* ecs, Entity tileEntity, IVec2 tilePosition, Vec2 point);

OptionalEntity<> findTileEntityAtPosition(const GameState* state, Vec2 position);

/*
* Remove (or destroy) the entity on the tile, if it exists.
* Probably shouldn't use this tbh, not a good function.
* @return A boolean representing whether the entity was destroyed.
*/
bool removeEntityOnTile(EntityWorld* ecs, Tile* tile);

bool placeEntityOnTile(EntityWorld* ecs, Tile* tile, Entity entity);

void forEachEntityInRange(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, const std::function<int(EntityT<EC::Position>)>& callback);

void forEachEntityNearPoint(const ComponentManager<EC::Position, const EC::Size>& ecs, const ChunkMap* chunkmap, Vec2 point, const std::function<int(EntityT<EC::Position>)>& callback);

void forEachChunkContainingBounds(const ChunkMap* chunkmap, Vec2 position, Vec2 size, const std::function<void(ChunkData*)>& callback);

void forEachEntityInBounds(const ComponentManager<const EC::Position, const EC::Size>& ecs, const ChunkMap* chunkmap, Vec2 position, Vec2 size, const std::function<void(EntityT<EC::Position>)>& callback);

OptionalEntity<EC::Position, EC::Size>
findFirstEntityAtPosition(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 position);

OptionalEntity<EC::Position>
findClosestEntityToPosition(const EntityWorld* ecs, const ChunkMap* chunkmap, Vec2 position);

bool pointInsideEntityBounds(Vec2 point, EntityT<EC::Position, EC::Size> entity, const ComponentManager<const EC::Position, const EC::Size>& ecs);

#endif