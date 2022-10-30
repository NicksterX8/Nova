#ifndef COMPONENT_POOL_INCLUDED
#define COMPONENT_POOL_INCLUDED

#include "Entity.hpp"
#include "ComponentMetadata/components.hpp"
#include "global.hpp"
#include "memory.hpp"

#define ECS_NULL_INDEX -1
#define ECS_MAX_COMPONENT_NAME_SIZE 256

namespace ECS {

constexpr Uint32 DEFAULT_COMPONENT_POOL_CAPACITY = 16;

struct 
alignas(64)
ComponentPool {
    size_t componentSize;
    Sint32 size; // how many used components exist in the components array
    Sint32 capacity; // number of reserved component memory spaces

    Component* components; // array of components
    EntityID* componentOwners; // array of components, same size as 'components'
    Sint32* entityComponentSet; // array indexable by entity id to get the index of the component owned by that entity
    
    const char* name; // human readable name of component type
    ComponentID id;

    
    /*
    ComponentPool(ComponentID componentID, size_t componentSize, Uint32 startCapacity)
    : componentSize(componentSize), size(0), capacity(startCapacity), name(NULL), id(componentID) {
        if (componentSize != 0) {
            components = (Component*)malloc(startCapacity * componentSize);
            componentOwners = (EntityID*)malloc(startCapacity * sizeof(EntityID));
            entityComponentSet = (Uint32*)malloc(MAX_ENTITIES * sizeof(Uint32));
            for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
                entityComponentSet[c] = ECS_NULL_INDEX;
            }
        }
    }
    */

    static ComponentPool Init(ComponentID componentID, size_t componentSize, Sint32 startCapacity) {
        if (componentSize != 0) {
            if (startCapacity < 0) startCapacity = 0;
            auto self = ComponentPool{
                .componentSize = componentSize,
                .size = 0,
                .capacity = startCapacity,
                .components = (Component*)malloc(startCapacity * componentSize),
                .componentOwners = (EntityID*)malloc(startCapacity * sizeof(EntityID)),
                .entityComponentSet = (Sint32*)malloc(MAX_ENTITIES * sizeof(Sint32)),
                .name = NULL,
                .id = componentID
            };
            if (!self.components || !self.componentOwners || !self.entityComponentSet) {
                LogCrash(CrashReason::MemoryFail, "Failed to allocate memory for component pool %d!", componentID);
            }
            for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
                self.entityComponentSet[c] = ECS_NULL_INDEX;
            }
            return self;
        }
        return ComponentPool{
            .componentSize = 0,
            .size = 0,
            .capacity = 0,
            .components = NULL,
            .componentOwners = NULL,
            .entityComponentSet = NULL,
            .name = NULL,
            .id = componentID
        };
    }

    // Destroy the component pool, deallocating all heap allocations
    void destroy() {
        free(components);
        free(componentOwners);
        free(entityComponentSet);
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

        LogError("Failed to get component %s for entity %u. Component index: %d.", name, entity, componentIndex);
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
            if (!reallocate(size+1, capacity*2)) {
                return -1;
            }
        }

        entityComponentSet[entity] = size;
        componentOwners[size] = entity;
        ++size;
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

    bool reallocate(Sint32 minCapacity, Sint32 preferredCapacity) {
        assert(minCapacity >= 0 && "can't have negative component pool capacity");
        minCapacity = MAX(size, minCapacity); // capacity can't be smaller than size
        preferredCapacity = MAX(minCapacity, preferredCapacity); // preferred cannot be smaller than the minimum
        // skip all the junk below if reallocating to 0
        if (minCapacity == 0 && capacity > 0) {
            free(components);
            free(componentOwners);
            capacity = 0;
            size = 0;
        }

        auto newComponents = ReallocMin(components, minCapacity, preferredCapacity, componentSize);
        components = (Component*)newComponents.ptr;
        
        //EntityID* newOwners = Mem::safe_min_realloc(componentOwners, minCapacity * sizeof(EntityID), preferredCapacity * sizeof(EntityID));
        auto newOwners = ReallocMin<EntityID>(componentOwners, minCapacity, newComponents.size);
        componentOwners = newOwners.ptr;
        
        capacity = newOwners.size;
        return true;
    }

    // convenience method
    bool reallocate(Sint32 capacity) {
        return reallocate(capacity, capacity);
    }
};

}

#endif