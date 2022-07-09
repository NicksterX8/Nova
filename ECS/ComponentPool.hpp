#ifndef COMPONENT_POOL_INCLUDED
#define COMPONENT_POOL_INCLUDED

#include "Entity.hpp"
#include "../ComponentMetadata/component.hpp"

#define NULL_COMPONENTPOOL_INDEX UINT32_MAX

/*
template<class T>
class ComponentPool {
public:
    T* components = NULL; // array of components
    Uint32 size = 0; // how many used components exist in the components array
    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity
    Uint32 entityComponentSetSize = 1;

    EntityID* componentOwners = NULL;

    Uint32 reserved = 0; // number of reserved component memory spaces

    Uint32 getSize() const {
        return size;
    }

    ComponentPool(Uint32 startSize) {
        reserved = startSize;
        components = (T*)malloc(startSize * sizeof(T));
        componentOwners = (EntityID*)malloc(startSize * sizeof(EntityID));
        entityComponentSet = (Uint32*)malloc(MAX_ENTITIES * sizeof(Uint32));
        for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
            entityComponentSet[c] = NULL_COMPONENTPOOL_INDEX;
        }
        
        entityComponentSetSize = MAX_ENTITIES;
    }

    void destroy() {
        free(components);
        free(componentOwners);
        free(entityComponentSet);
    }

    T* get(EntityID entity) const {        
        Uint32 componentIndex = entityComponentSet[entity];

        // this is optimized to just check if index is greater than size rather than two separate checks
        if (componentIndex > size) {
            Log.Error("ComponentPool(%s)::get : componentIndex (%d) of entityComponentSet[%d] is greater than size or null: %d", getComponentName<T>(), componentIndex, entity, size);
            return NULL;
        }

        return &components[componentIndex];
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        if (entityComponentSet[entity] != NULL_COMPONENTPOOL_INDEX) {
            Log.Error("ComponentPool(%s)::add : Entity %u already has this component! Aborting.", getComponentName<T>(), entity);
            return -1;
        }

        //Log("Adding component %d to entity %d", getID<T>(), entity);
        if (size >= reserved) {
            // double in size every time it runs out of space
            // Log("resizing pool, old size: %d, new size: %d, reserved: %d", size, reserved * 2, reserved);
            resize(reserved * 2);
        }
        entityComponentSet[entity] = size;
        componentOwners[size] = entity;
        size++;
        return 0;
    }

    // returns 0 on success, -1 on failure.
    int remove(EntityID entity) {
        if (entity == NULL_ENTITY_ID) {
            Log.Error("ComponentPool(%s)::remove : Entity passed was NULL!", getComponentName<T>());
            return -1;
        }

        //Log("Removing component %d of entity %d", getID<T>(), entity);
        if (entityComponentSet[entity] == NULL_COMPONENTPOOL_INDEX || size == 0) {
            Log.Error("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %d, component index: %d, size: %d", getComponentName<T>(), entity, entityComponentSet[entity], size);
            return -1;
        }

        Uint32 lastComponentIndex = size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner == NULL_ENTITY_ID) {
            Log.Error("Last component owner is NULL while removing entity %d!", entity);
            return -1;
        }
        // dont need to switch data around if the entity is the last entity.
        // Also it would be undefined behavior to copy something to itself
        if (lastComponentOwner != entity) {
            T* entityComponent = get(entity); // the component owned by the entity being removed
        
            T* last = &components[lastComponentIndex]; // last component in the array
            memcpy(entityComponent, last, sizeof(T));
            // update the address to show it the last component's new position
            
            
            // move the entity owner over to reflect the new position of the (formerly) last component
            if (entityComponentSet[entity] > size) {
                Log.Error("Component index (%u) of entity %u is greater than componentpool size %u!", entityComponentSet[entity], entity, size);
            }
            entityComponentSet[lastComponentOwner] = entityComponentSet[entity];
            componentOwners[entityComponentSet[entity]] = lastComponentOwner;
        }
        
        // update it to be empty
        entityComponentSet[entity] = NULL_COMPONENTPOOL_INDEX;
        componentOwners[lastComponentIndex] = NULL_ENTITY_ID;
        size--;
        return 0;
    }

    void iterate(std::function<void(T&)> callback) const {
        for (int c = size-1; c >= 0; c--) {
            callback(components[c]);
        }
    }

    void resize(Uint32 newSize) {
        if (newSize < size) {
            // bad things
            Log("Warning: component pool was called with a resize value too small to contain all entities.\n"
                "old size: %d, new size: %d. Aborting.", size, newSize);
            return;
        }

        T* newComponents = (T*)realloc(components, newSize * sizeof(T));
        if (!newComponents) {
            Log.Error("Failed to realloc component pool! Component id: %d", getID<T>());
            newComponents = (T*)malloc(newSize * sizeof(T));
            memcpy(newComponents, components, size * sizeof(T));
            free(components);
        }
        components = newComponents;
        
        EntityID* newOwners = (EntityID*)realloc(componentOwners, newSize * sizeof(EntityID));
        if (!newOwners) {
            newOwners = (EntityID*)malloc(newSize * sizeof(EntityID));
            memcpy(newOwners, componentOwners, size * sizeof(EntityID));
            free(componentOwners);
        }
        componentOwners = newOwners;
        

        reserved = newSize;
    }
};
*/

class NewComponentPool {
public:
    ComponentID id;
    size_t componentSize;

    void* components = NULL; // array of components
    Uint32 _size = 0; // how many used components exist in the components array
    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity

    EntityID* componentOwners = NULL;

    Uint32 reserved = 0; // number of reserved component memory spaces

    Uint32 size() const {
        return _size;
    }

    NewComponentPool(ComponentID componentID, size_t componentSize, Uint32 startSize)
    : id(componentID), componentSize(componentSize) {
        reserved = startSize;
        components = malloc(startSize * componentSize);
        componentOwners = (EntityID*)malloc(startSize * sizeof(EntityID));
        entityComponentSet = (Uint32*)malloc(MAX_ENTITIES * sizeof(Uint32));
        for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
            entityComponentSet[c] = NULL_COMPONENTPOOL_INDEX;
        }
    }

    void destroy() {
        free(components);
        free(componentOwners);
        free(entityComponentSet);
    }

    void* atIndex(Uint32 index) const {
        char* componentsData = static_cast<char*>(components);
        return &componentsData[index * componentSize];
    }

    void* get(EntityID entity) const {        
        Uint32 componentIndex = entityComponentSet[entity];

        // this is optimized to just check if index is greater than size rather than two separate checks
        if (componentIndex > _size) {
            Log.Error("ComponentPool(%s)::get : componentIndex (%d) of entityComponentSet[%d] is greater than size or null: %d", componentNames[id], componentIndex, entity, _size);
            return NULL;
        }

        return atIndex(componentIndex);
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        if (entity == NULL_ENTITY_ID) {
            Log.Error("ComponentPool(%s)::add : Entity is NULL!", componentNames[id]);
            return -1;
        }

        if (entityComponentSet[entity] != NULL_COMPONENTPOOL_INDEX) {
            Log.Error("ComponentPool(%s)::add : Entity %u already has this component! Aborting.", componentNames[id], entity);
            return -1;
        }

        //Log("Adding component %d to entity %d", getID<T>(), entity);
        if (_size >= reserved) {
            // double in size every time it runs out of space
            // Log("resizing pool, old size: %d, new size: %d, reserved: %d", size, reserved * 2, reserved);
            resize(reserved * 2);
        }
        entityComponentSet[entity] = _size;
        componentOwners[_size] = entity;
        _size++;
        return 0;
    }

    // returns 0 on success, -1 on failure.
    int remove(EntityID entity) {
        if (entityComponentSet[entity] > _size) {
            Log.Error("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %d, component index: %d, size: %d", componentNames[id], entity, entityComponentSet[entity], _size);
            return -1;
        }

        Uint32 lastComponentIndex = _size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner == NULL_ENTITY_ID) {
            Log.Error("Last component owner is NULL while removing entity %d!", entity);
            return -1;
        }
        // dont need to switch data around if the entity is the last entity.
        // Also it would be undefined behavior to copy something to itself
        if (lastComponentOwner != entity) {
            void* entityComponent = get(entity); // the component owned by the entity being removed
            void* last = atIndex(lastComponentIndex); // last component in the array
            memcpy(entityComponent, last, componentSize);
            
            // move the entity owner over to reflect the new position of the (formerly) last component
            // update the address to show it the last component's new position
            entityComponentSet[lastComponentOwner] = entityComponentSet[entity];
            componentOwners[entityComponentSet[entity]] = lastComponentOwner;
        }
        
        // update it to be empty
        entityComponentSet[entity] = NULL_COMPONENTPOOL_INDEX;
        componentOwners[lastComponentIndex] = NULL_ENTITY_ID;
        _size--;
        return 0;
    }

    void iterate(std::function<void(void*)> callback) const {
        for (int c = _size-1; c >= 0; c--) {
            callback(atIndex(c));
        }
    }

    int resize(Uint32 newSize) {
        if (newSize < _size) {
            // bad things
            Log.Error("Warning: component pool was called with a resize value too small to contain all entities.\n"
                "old size: %d, new size: %d. Aborting.", _size, newSize);
            return -1;
        }

        void* newComponents = realloc(components, newSize * componentSize);
        if (!newComponents) {
            Log.Error("Failed to realloc component pool %s!", componentNames[id]);
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
};

using ComponentPool = NewComponentPool;

#endif