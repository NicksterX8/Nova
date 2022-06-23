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
    ComponentFlags newSignature = manager.entityComponents(entity.id);
    for (Uint32 id = 0; id < NUM_SYSTEMS; id++) {
        bool shouldBeInSystem = systems[id]->Query(newSignature);
        bool isInSystem = systems[id]->Query(oldSignature);
        if (shouldBeInSystem) {
            if (!isInSystem) {
                systems[id]->AddEntity(entity);
            }
            
        } else {
            if (isInSystem) {
                systems[id]->RemoveEntity(entity);
            }
        }
            
    }
}

void ECS::ScheduleRemove(Entity entity) {
    entitiesToBeRemoved.push_back(entity);
}

EntityManager* ECS::em() {
    return &manager;
}

const EntityManager* ECS::em() const {
    return &manager;
}