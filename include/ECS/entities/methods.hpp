#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "ECS/components/components.hpp"
#include "ECS/EntityWorld.hpp"
#include "Chunks.hpp"

namespace Entities {

void scaleGuy(GameState* state, Entity guy, float scale);

/* 
 * Find an entity with a given name. If multiple entities have the same name,
 * the entity will be chosen randomly
 */
Entity findNamedEntity(const char* name, const EntityWorld* ecs);

inline Entity clone(EntityWorld* ecs, Entity entity) {
    if (!ecs->EntityExists(entity)) return NullEntity;

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

inline void giveName(Entity entity, const char* name, EntityWorld* ecs) {
    ecs->Add<EC::Nametag>(entity, EC::Nametag(name));
}

}

void entityCreated(GameState* state, Entity entity);

void entityViewChanged(ChunkMap* chunkmap, Entity entity, Vec2 newPos, Vec2 oldPos, Box newViewbox, Box oldViewbox, bool justMade);
void entityPositionChanged(GameState* state, Entity entity, Vec2 oldPos);
void entityViewboxChanged(GameState* state, Entity entity, Box oldViewbox);
void entityViewAndPosChanged(GameState* state, Entity entity, Vec2 oldPos, Box oldViewbox);

#endif