#ifndef WORLD_FUNCTIONS
#define WORLD_FUNCTIONS

#include "EntityWorld.hpp"
#include "components/components.hpp"

struct ChunkMap;
struct GameState;

namespace World {

inline float getLayerHeight(int layer) {
    return layer * 0.05f - 0.5f;
}

inline float getEntityHeight(ECS::EntityID id, int renderLayer) {
    return getLayerHeight(renderLayer) + id * 0.0000001f;
}

bool pointInEntity(Vec2 point, Entity entity, const EntityWorld& ecs);

Entity findPlayerFocusedEntity(const EntityWorld& ecs, const ChunkMap& chunkmap, Vec2 playerMousePos);

Box getEntityViewBoxBounds(const EntityWorld* ecs, Entity entity);

void setEventCallbacks(EntityWorld& ecs, ChunkMap& chunkmap);

void forEachEntityInRange(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 pos, float radius, const std::function<int(Entity)>& callback);
void forEachEntityNearPoint(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 point, const std::function<int(Entity)>& callback);
void forEachEntityInBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, const std::function<void(Entity)>& callback);
Entity findClosestEntityToPosition(const EntityWorld& ecs, const ChunkMap* chunkmap, Vec2 position);

SmallVectorA<Entity, VirtualAllocator, 4> getAllEntitiesInBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, VirtualAllocator allocator);
SmallVectorA<Entity, VirtualAllocator, 0> getAllEntitiesNearBounds(const EntityWorld& ecs, const ChunkMap* chunkmap, Boxf bounds, VirtualAllocator allocator);

void entityCreated(GameState* state, Entity entity);

void entityViewChanged(ChunkMap* chunkmap, Entity entity, Vec2 newPos, Vec2 oldPos, Box newViewbox, Box oldViewbox, bool justMade);
void entityPositionChanged(GameState* state, Entity entity, Vec2 oldPos);
void entityViewboxChanged(GameState* state, Entity entity, Box oldViewbox);
void entityViewAndPosChanged(GameState* state, Entity entity, Vec2 oldPos, Box oldViewbox);

}

#endif