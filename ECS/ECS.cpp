#include "ECS.hpp"

void ECS::destroy() {
    manager.destroy();
}

/*
inline Uint32 ECS::numLiveEntities() {
    return manager.numLiveEntities();
}

inline bool ECS::EntityLives(Entity entity) {
    return manager.EntityLives(entity);
}

inline Entity ECS::New() {
    return manager.New();
}
*/

void ECS::entitySignatureChanged(Entity entity, ComponentFlags oldSignature) {
    for (Uint32 id = 0; id < NUM_SYSTEMS; id++) {
        systems[id]->RemoveEntity(entity);
    }

    ComponentFlags newSignature = manager.entityComponents(entity.id);
    for (Uint32 id = 0; id < NUM_SYSTEMS; id++) {
        if (!systems[id]->Query(oldSignature) && systems[id]->Query(newSignature)) {
            systems[id]->AddEntity(entity);
        }
    }
}

void ECS::ScheduleRemove(Entity entity) {
    entitiesToBeRemoved.push_back(entity);
}

EntityManager* ECS::em() {
    return &manager;
}