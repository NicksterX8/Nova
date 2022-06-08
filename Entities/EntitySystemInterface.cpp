#include "EntitySystemInterface.hpp"
#include "EntityManager.hpp"

EntitySystemInterface::EntitySystemInterface() {
    nEntities = 0;
    reservedEntities = 100;

    entities = (_Entity*)malloc(100 * sizeof(_Entity));
}

EntitySystemInterface::EntitySystemInterface(Uint32 reserve) {
    nEntities = 0;
    reservedEntities = reserve;

    entities = (_Entity*)malloc(reserve * sizeof(_Entity));
}

void EntitySystemInterface::AddEntity(Entity entity) {
    if (nEntities >= reservedEntities) {
        // need to resize
        resize(reservedEntities * 2);
    }

    entities[nEntities] = entity;
    nEntities++;
}


int EntitySystemInterface::RemoveEntity(Uint32 localIndex) {
    if (localIndex >= nEntities) {
        LogError("EntitySystem::RemoveEntity : localIndex out of bounds. index: %u, nEntities: %u", localIndex, nEntities);
        return 1;
    }
    entities[localIndex] = entities[nEntities-1];
    nEntities--;
    return 0;
}

int EntitySystemInterface::RemoveEntity(Entity entity) {
    if (nEntities == 0) {
        //LogError("Tried to remove an entity from a system that has no entities!");
        return -1;
    }

    for (Uint32 e = nEntities-1;; e--) {
        if (entity.id == entities[e].id) {
            RemoveEntity(e);
            return 0;
        }

        if (e == 0) break;
    }
    return -1;
}

void EntitySystemInterface::EntitiesWereRemoved(EntityManager* em) {
    for (Uint32 e = 0; e < nEntities; e++) {
        _Entity entity = entities[e];
        
        if (!em->EntityLives(entity)) {
            RemoveEntity(e);
        }
    }
}

void EntitySystemInterface::RemoveAllEntities() {
    nEntities = 0;
}

void EntitySystemInterface::resize(Uint32 reserve) {
    _Entity *newEntities = (_Entity*)malloc(reserve * sizeof(_Entity));
    memcpy(newEntities, entities, reservedEntities * sizeof(_Entity));
    free(entities);

    entities = newEntities;
    
    reservedEntities = reserve;
}