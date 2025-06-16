#ifndef ENTITY_METHODS_INCLUDED
#define ENTITY_METHODS_INCLUDED

#include "world/components/components.hpp"
#include "world/EntityWorld.hpp"
#include "Chunks.hpp"

struct GameState;

namespace World {

namespace Entities {

inline Entity clone(EntityWorld* ecs, Entity base) {
    return NullEntity;
}

void scaleGuy(GameState* state, Entity guy, float scale);

/* 
 * Find an entity with a given name. If multiple entities have the same name,
 * the entity will be chosen randomly
 */
Entity findNamedEntity(const char* name, const EntityWorld* ecs);

/* Throw the entity, adding motion and angle motion components.
 */
void throwEntity(EntityWorld& ecs, Entity entity, Vec2 target, float speed);

inline void giveName(Entity entity, const char* name, EntityWorld* ecs) {
    ecs->Add<EC::Nametag>(entity, EC::Nametag(name));
}

}

void entityCreated(GameState* state, Entity entity);

void entityViewChanged(ChunkMap* chunkmap, Entity entity, Vec2 newPos, Vec2 oldPos, Box newViewbox, Box oldViewbox, bool justMade);
void entityPositionChanged(GameState* state, Entity entity, Vec2 oldPos);
void entityViewboxChanged(GameState* state, Entity entity, Box oldViewbox);
void entityViewAndPosChanged(GameState* state, Entity entity, Vec2 oldPos, Box oldViewbox);

}

#endif