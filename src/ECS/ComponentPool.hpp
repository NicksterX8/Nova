#ifndef COMPONENT_POOL_INCLUDED
#define COMPONENT_POOL_INCLUDED

#include "Entity.hpp"
#include "../../ComponentMetadata/component.hpp"

#define ECS_NULL_INDEX UINT32_MAX
#define ECS_MAX_COMPONENT_NAME_SIZE 256

namespace ECS {

struct ComponentPool {
    size_t componentSize;
    Uint32 size = 0; // how many used components exist in the components array
    Uint32 capacity = 0; // number of reserved component memory spaces

    Component* components = NULL; // array of components
    EntityID* componentOwners = NULL;

    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity

    ComponentID id;
    char name[ECS_MAX_COMPONENT_NAME_SIZE] = "null";

    ComponentPool(ComponentID componentID, size_t componentSize, Uint32 startCapacity)
    : componentSize(componentSize), id(componentID) {
        if (componentSize != 0) {
            capacity = startCapacity;
            components = (Component*)malloc(startCapacity * componentSize);
            componentOwners = (EntityID*)malloc(startCapacity * sizeof(EntityID));
            entityComponentSet = (Uint32*)malloc(MAX_ENTITIES * sizeof(Uint32));
            for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
                entityComponentSet[c] = ECS_NULL_INDEX;
            }
        }
    }

    // Destroy the component pool, deallocating all heap allocations
    void destroy() {
        free(components);
        free(componentOwners);
        free(entityComponentSet);
    }

    // set the name of the component type / pool, currently used only for debugging. Max size of 256, after that name is cut off.
    void setName(const char* componentName) {
        strncpy(name, componentName, ECS_MAX_COMPONENT_NAME_SIZE);
    }

    inline Component* atIndex(Uint32 index) const {
        char* componentsData = (char*)components;
        return (Component*)(&componentsData[index * componentSize]);
    }

    Component* get(EntityID entity) const {
        Uint32 componentIndex = entityComponentSet[entity];

        // this is optimized to just check if index is greater than size rather than two separate checks
        if (componentIndex < size) {
            return atIndex(componentIndex);
        }

        LogError("Failed to get component %s for entity %u. Component index: %u.", name, entity, componentIndex);
        return NULL;
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        if (entity >= NULL_ENTITY_ID) {
            LogError("ComponentPool(%s)::add : Entity is NULL!", name);
            return -1;
        }
        if (entityComponentSet[entity] != ECS_NULL_INDEX) {
            LogError("ComponentPool(%s)::add : Entity already has this component! Entity: %u", name, entity);
            return -1;
        }
        if (size >= capacity) {
            // double in size every time it runs out of space
            if (reallocate(capacity * 2) != 0) {
                return -1;
            }
        }

        entityComponentSet[entity] = size;
        componentOwners[size] = entity;
        size++;
        return 0;
    }

    // returns 0 on success, -1 on failure.
    int remove(EntityID entity) {
        Uint32 entityComponentIndex = entityComponentSet[entity];
        if (entityComponentIndex > size) {
            LogError("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %u, component index: %u, size: %u", name, entity, entityComponentSet[entity], size);
            return -1;
        }

        Uint32 lastComponentIndex = size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner >= NULL_ENTITY_ID) {
            LogError("ComponentPool(%s)::remove : Last component owner is NULL while removing entity %u!", name, entity);
            return -1;
        }
        // dont need to switch data around if the entity is the last entity.
        // Also it would be undefined behavior to copy something to itself
        if (lastComponentOwner != entity) {
            void* entityComponent = atIndex(entityComponentIndex); // the component owned by the entity being removed
            void* last = atIndex(lastComponentIndex); // last component in the array
            memcpy(entityComponent, last, componentSize);
            
            // move the entity owner over to reflect the new position of the (formerly) last component
            // update the address to show it the last component's new position
            entityComponentSet[lastComponentOwner] = entityComponentIndex;
            componentOwners[entityComponentIndex] = lastComponentOwner;
        }
        
        // update it to be empty
        entityComponentSet[entity] = ECS_NULL_INDEX;
        componentOwners[lastComponentIndex] = NULL_ENTITY_ID;
        size--;
        return 0;
    }

    int reallocate(Uint32 newCapacity) {
        newCapacity = (newCapacity > size) ? newCapacity : size; // can't reallocate smaller than current size

        Component* newComponents = (Component*)realloc(components, newCapacity * componentSize);
        if (!newComponents) {
            LogWarn("Failed to reallocate space for components for component pool %s", name);
            return -1;
        }
        components = newComponents;
        
        EntityID* newOwners = (EntityID*)realloc(componentOwners, newCapacity * sizeof(EntityID));
        if (!newOwners) {
            LogWarn("Failed to reallocate space for components for component pool %s", name);
            return -1;
        }
        componentOwners = newOwners;

        capacity = newCapacity;

        return newCapacity;
    }

    void iterateComponents(std::function<void(void*)> callback) const {
        for (int c = size-1; c >= 0; c--) {
            callback(atIndex(c));
        }
    }

    void iterateEntities(std::function<void(EntityID)> callback) const {
        for (int c = size-1; c >= 0; c--) {
            callback(componentOwners[c]);
        }
    }
};

}

#endif