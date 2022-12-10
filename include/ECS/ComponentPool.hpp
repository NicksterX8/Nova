#ifndef ECS_COMPONENTPOOL_INCLUDED
#define ECS_COMPONENTPOOL_INCLUDED

#include "ECS/generic/ComponentPool.hpp"
#include "Entity.hpp"

namespace ECS {

using ComponentPoolBase = SECS::ComponentPool<EntityID, MAX_ENTITIES, NULL_ENTITY_ID>;
struct ComponentPool : ComponentPoolBase {
    ComponentID id;
    const char* name;

    ComponentPool(ComponentID id, int componentTypeSize) : ComponentPoolBase(componentTypeSize), id(id) {

    }
};

}

#endif