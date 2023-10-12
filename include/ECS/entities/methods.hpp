#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "ECS/components/components.hpp"
#include "ECS/EntityWorld.hpp"
#include "Chunks.hpp"

namespace Entities {

inline Entity clone(EntityWorld* ecs, Entity entity) {
    ecs->StartDeferringEvents();

    ComponentFlags signature = ecs->EntitySignature(entity);
    const char* type = ecs->Get<EC::EntityTypeEC>(entity)->name;
    Entity newEntity = ecs->New(type);
    ecs->AddSignature(newEntity, signature);
    signature.forEachSet([&](auto componentID){
        auto size = ecs->getComponentSize(componentID);
        const void* oldComponent = ecs->Get(componentID, entity);
        void* newComponent = ecs->Get(componentID, newEntity);
        memcpy(newComponent, oldComponent, size);
    });

    ecs->StopDeferringEvents();

    return newEntity;
}

/* Throw the entity, adding motion and angle motion components.
 */
void throwEntity(EntityWorld* ecs, Entity entity, Vec2 target, float speed);

}

void entityCreated(GameState* state, Entity entity);

void entitySizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::ViewBox> entity, Vec2 oldSize);
void entityPositionChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::ViewBox> entity, Vec2 oldPos);
void entityPositionAndSizeChanged(ChunkMap* chunkmap, const EntityWorld* ecs, EntityT<EC::ViewBox> entity, Vec2 oldPos, Vec2 oldSize);

#endif