#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "ECS/components/components.hpp"
#include "ECS/EntityWorld.hpp"
#include "Chunks.hpp"

namespace Entities {

inline Entity clone(EntityWorld* ecs, Entity entity) {
    auto signature = ecs->EntitySignature(entity);
    const char* type = ecs->Get<EC::EntityTypeEC>(entity)->name;
    Entity newEntity = ecs->New(type);
    ecs->AddSignature(newEntity, signature);
    signature.forEachSet([&](auto componentID){
        auto size = ecs->getComponentSize(componentID);
        const void* oldComponent = ecs->Get(componentID, entity);
        void* newComponent = ecs->Get(componentID, entity);
        memcpy(newComponent, oldComponent, size);
    });

    // TODO:
    UNFINISHED_CRASH();
}

/* Throw the entity, adding motion and angle motion components.
 */
void throwEntity(EntityWorld* ecs, Entity entity, Vec2 target, float speed);

inline bool tryOccupyTileAt(Entity entity, Vec2 position, ChunkMap* chunkmap, const EntityWorld* ecs) {
    Tile* tile = getTileAtPosition(*chunkmap, position);
    // TODO: entity collision stuff

    return false;
}

inline bool tryOccupyTile(EntityT<EC::Position> entity, ChunkMap* chunkmap, const EntityWorld* ecs) {
    Vec2 position = ecs->Get<EC::Position>(entity)->vec2();
    return tryOccupyTileAt(entity, position, chunkmap, ecs);
}

}

void entityCreated(GameState* state, Entity entity);

void entitySizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position, EC::Size> entity, Vec2 oldSize);
void entityPositionChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position> entity, Vec2 oldPos);
void entityPositionAndSizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::Position, EC::Size> entity, Vec2 oldPos, Vec2 oldSize);

#endif