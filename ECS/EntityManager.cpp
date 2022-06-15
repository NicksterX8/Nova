#include "EntityManager.hpp"

EntityManager::EntityManager() {}

void EntityManager::destroy() {
    destroyPools();
}

Uint32 EntityManager::numLiveEntities() const {
    return liveEntities;
}

bool EntityManager::EntityLives(_Entity entity) const {
    if (entity.id != NULL_ENTITY) {
        Uint32 index = indices[entity.id];
        if (index != NULL_ENTITY) {
            return (entities[index].version == entity.version);
        }
    }
    
    return false;
}

_Entity EntityManager::New() {
    if (liveEntities < MAX_ENTITIES - 1 && freeEntities.size() > 0) {
        // get top entity from free entity stack
        _Entity entity = freeEntities.back();
        if (entity.id == NULL_ENTITY) {
            LogError("Entity from freeEntities was NULL! EntityID: %d", entity.id);
        }
        freeEntities.pop_back();

        // add free entity to end of entities array
        Uint32 index = liveEntities;
        entities[index] = entity;
        indices[entity.id] = index; // update the indices array to tell where the entity is stored
        entityComponentFlags[entity.id] = ComponentFlags(false); // entity starts with no components
        liveEntities++;

        return entity;
    }
    
    LogError("Ran out of entity space in ECS! Live entities: %u, freeEntities size: %lu. Aborting with return NULL_ENTITY.\n", liveEntities, freeEntities.size());
    return {NULL_ENTITY, 0};
}

_Entity EntityManager::getEntityByIndex(Uint32 entityIndex) {
    if (entityIndex >= liveEntities) {
        LogError("ECS::getEntityByIndex : index out out bounds! Index : %u, liveEntities: %u", entityIndex, liveEntities);
        return entities[0];
    }
    return entities[entityIndex];
}

ComponentFlags EntityManager::entityComponents(EntityID entityID) const {
    return entityComponentFlags[entityID];
}  