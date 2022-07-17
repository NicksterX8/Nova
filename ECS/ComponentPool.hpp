#ifndef COMPONENT_POOL_INCLUDED
#define COMPONENT_POOL_INCLUDED

#include "Entity.hpp"
#include "../ComponentMetadata/component.hpp"

#define ECS_NULL_INDEX UINT32_MAX
#define ECS_MAX_COMPONENT_NAME_SIZE 256

namespace ECS {

class ComponentPool {
public:
    size_t componentSize;
    Uint32 _size = 0; // how many used components exist in the components array
    Uint32 reserved = 0; // number of reserved component memory spaces

    void* components = NULL; // array of components
    EntityID* componentOwners = NULL;

    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity

    ComponentID id;
    char name[ECS_MAX_COMPONENT_NAME_SIZE] = "null";

    Uint32 size() const {
        return _size;
    }

    ComponentPool(ComponentID componentID, size_t componentSize, Uint32 startSize)
    : componentSize(componentSize), id(componentID) {
        if (componentSize != 0) {
            reserved = startSize;
            components = malloc(startSize * componentSize);
            componentOwners = (EntityID*)malloc(startSize * sizeof(EntityID));
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

    inline void* atIndex(Uint32 index) const {
        char* componentsData = static_cast<char*>(components);
        return &componentsData[index * componentSize];
    }

    void* get(EntityID entity) const {
        Uint32 componentIndex = entityComponentSet[entity];

        // this is optimized to just check if index is greater than size rather than two separate checks
        if (componentIndex < _size) {
            return atIndex(componentIndex);
        }

        Log.Error("ComponentPool(%s)::get : componentIndex (%d) of entityComponentSet[%d] is greater than size or null: %d", name, componentIndex, entity, _size);
        return NULL;
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        if (entity >= NULL_ENTITY_ID) {
            Log.Error("ComponentPool(%s)::add : Entity is NULL!", name);
            return -1;
        }
        if (entityComponentSet[entity] != ECS_NULL_INDEX) {
            Log.Error("ComponentPool(%s)::add : Entity already has this component! Entity: %u", name, entity);
            return -1;
        }
        if (_size >= reserved) {
            // double in size every time it runs out of space
            // Log("resizing pool, old size: %d, new size: %d, reserved: %d", size, reserved * 2, reserved);
            if (resize(reserved * 2)) {
                return -1;
            }
        }

        entityComponentSet[entity] = _size;
        componentOwners[_size] = entity;
        _size++;
        return 0;
    }

    // returns 0 on success, -1 on failure.
    int remove(EntityID entity) {
        Uint32 entityComponentIndex = entityComponentSet[entity];
        if (entityComponentIndex > _size) {
            Log.Error("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %u, component index: %u, size: %u", name, entity, entityComponentSet[entity], _size);
            return -1;
        }

        Uint32 lastComponentIndex = _size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner >= NULL_ENTITY_ID) {
            Log.Error("ComponentPool(%s)::remove : Last component owner is NULL while removing entity %u!", name, entity);
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
        _size--;
        return 0;
    }

    int resize(Uint32 newSize) {
        if (newSize < _size) {
            // bad things
            Log.Error("ComponentPool(%s)::resize : New size is too small to contain all entities!\n"
                "Old size: %d, New size: %d. Aborting.", name, _size, newSize);
            return -1;
        }

        if (newSize > reserved/2 && newSize < reserved) {
            // do nothing as the size is already large enough and shortening it wouldn't save that much memory
            return 0;
        }

        void* newComponents = realloc(components, newSize * componentSize);
        if (!newComponents) {
            Log.Warn("ComponentPool(%s)::resize : Realloc returned null!", name);
            newComponents = malloc(newSize * componentSize);
            memcpy(newComponents, components, _size * componentSize);
            free(components);
        }
        components = newComponents;
        
        EntityID* newOwners = (EntityID*)realloc(componentOwners, newSize * sizeof(EntityID));
        if (!newOwners) {
            newOwners = (EntityID*)malloc(newSize * sizeof(EntityID));
            memcpy(newOwners, componentOwners, _size * sizeof(EntityID));
            free(componentOwners);
        }
        componentOwners = newOwners;
        reserved = newSize;

        return 0;
    }

    void iterateComponents(std::function<void(void*)> callback) const {
        for (int c = _size-1; c >= 0; c--) {
            callback(atIndex(c));
        }
    }

    void iterateEntities(std::function<void(EntityID)> callback) const {
        for (int c = _size-1; c >= 0; c--) {
            callback(componentOwners[c]);
        }
    }
};

}

#endif