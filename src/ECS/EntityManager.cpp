#include "ECS/EntityManager.hpp"

namespace ECS {

EntityManager::EntityManager() {
    freeEntities = My::Vec<EntityID>::WithCapacity(MAX_ENTITIES-1);
    entities = (Entity*)malloc(MAX_ENTITIES * sizeof(Entity));
    entityDataList = (EntityData*)malloc(MAX_ENTITIES * sizeof(EntityData));
}

void EntityManager::destroy() {
    for (ComponentID id = 0; id < nComponents; id++) {
        pools[id].destroy();
    }
    delete pools;

    freeEntities.destroy();
    free(entities);
    free(entityDataList);
}

Entity EntityManager::New() {
    if (entityCount >= NULL_ENTITY_ID || freeEntities.size < 0) {
        LogCritical("Ran out of entity space in ECS! Live entities: %u, freeEntities size: %lu. Aborting.\n", entityCount, freeEntities.size);
        return NullEntity;
    }

    // get top entity from free entity stack
    EntityID entityID = freeEntities.back();
    if (entityID == NULL_ENTITY_ID) {
        LogError("Entity from freeEntities was NULL! EntityID: %u", entityID);
    }
    freeEntities.pop();

    EntityData* entityData = &entityDataList[entityID];
    EntityVersion entityVersion = entityData->version;

    // add free entity to end of entities array
    Uint32 index = entityCount;
    entities[index] = Entity(entityID, entityVersion);
    entityData->index = index; // update the entity's location in the entity array
    entityData->flags = ComponentFlags(0); // I set this when an entity is destroyed and when a new one is created, so it's redundant here or at destruction.
    entityCount++;

    return Entity(entityID, entityVersion);
}

int EntityManager::Destroy(Entity entity) {
    EntityData* entityData = &entityDataList[entity.id];

    if (!EntityExists(entity)) {
        LogError("ECS::Remove %s : Attempted to remove a non-existent entity! Returning -1. Entity: %s", entity.DebugStr());
        return -1;
    }

    int code = 0;
    for (Uint32 id = 0; id < nComponents; id++) {
        if (entityData->flags[id])
            code |= pools[id].remove(entity.id);
    }

    // swap last entity with this entity for 100% entity packing in array
    Uint32 lastEntityIndex = entityCount-1;
    Entity lastEntity = entities[lastEntityIndex];

    //Uint32 entityIndex = indices[entity.id]; // the index of the entity being removed
    // move last entity to removed entity position in array
    entities[entityData->index] = lastEntity;
    // tell indices where the (formerly) last entity now is
    entityDataList[lastEntity.id].index = entityData->index;

    // Version goes up one everytime it's removed
    entityDataList[entity.id].version += 1;
    freeEntities.push(entity.id);

    entityCount--;

    // set now unused last entity position data to null
    entities[entityCount].id = NULL_ENTITY_ID;

    // set other destroyed entity data back to null just in case
    entityData->index = ECS_NULL_INDEX;
    entityData->flags = ComponentFlags(0);

    return code;
}

Entity EntityManager::getEntityByIndex(Uint32 entityIndex) const {
    if (entityIndex >= entityCount) {
        LogError("ECS::getEntityByIndex : index out out bounds! Index : %u, liveEntities: %u", entityIndex, entityCount);
        return NullEntity;
    }
    return entities[entityIndex];
}

ComponentFlags EntityManager::EntitySignature(EntityID entityID) const {
    return entityDataList[entityID].flags;
}

}