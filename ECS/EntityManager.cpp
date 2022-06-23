#include "EntityManager.hpp"

EntityManager::EntityManager() {}

void EntityManager::destroy() {
    destroyPools();
}

Uint32 EntityManager::numLiveEntities() const {
    return liveEntities;
}

bool EntityManager::EntityLives(_Entity entity) const {
    // TODO: Optimize 
    if (entity.IsValid()) {
        Uint32 index = indices[entity.id];
        if (index != NULL_ENTITY_ID) {
            return (entities[index].version == entity.version);
        }
    }
    
    return false;
}

_Entity EntityManager::New() {
    if (liveEntities >= MAX_ENTITIES || freeEntities.size() < 0) {
        Log.Critical("Ran out of entity space in ECS! Live entities: %u, freeEntities size: %lu. Aborting.\n", liveEntities, freeEntities.size());
        abort();
    }

    // get top entity from free entity stack
    EntityBase entity = freeEntities.back();
    if (!entity.IsValid()) {
        Log.Error("Entity from freeEntities was NULL! EntityID: %d", entity.id);
    }
    freeEntities.pop_back();

    // add free entity to end of entities array
    Uint32 index = liveEntities;
    entities[index] = entity;
    indices[entity.id] = index; // update the indices array to tell where the entity is stored
    entityComponentFlags[entity.id] = ComponentFlags(false); // entity starts with no components
    liveEntities++;

    return baseToEntity(entity);
}

_Entity EntityManager::getEntityByIndex(Uint32 entityIndex) {
    if (entityIndex >= liveEntities) {
        Log.Error("ECS::getEntityByIndex : index out out bounds! Index : %u, liveEntities: %u", entityIndex, liveEntities);
        return baseToEntity(entities[0]);
    }
    return baseToEntity(entities[entityIndex]);
}

ComponentFlags EntityManager::entityComponents(EntityID entityID) const {
    return entityComponentFlags[entityID];
}  