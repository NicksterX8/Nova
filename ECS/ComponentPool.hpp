#ifndef COMPONENT_POOL_INCLUDED
#define COMPONENT_POOL_INCLUDED

#include "Entity.hpp"
#include "../ComponentMetadata/component.hpp"

template<class T>
class ComponentPool {
    T* components = NULL; // array of components
    Uint32 size = 0; // how many used components exist in the components array
    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity
    Uint32 entityComponentSetSize = 1;

    #define NULL_COMPONENTPOOL_INDEX UINT32_MAX

    EntityID* componentOwners = NULL;
public:
    Uint32 reserved = 0; // number of reserved component memory spaces

    Uint32 getSize() {
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
        // this check is removed to increase performance
        
        if (entity > entityComponentSetSize) {
            Log("ComponentPool(%s)::get was called with an entity (%u) higher than entityComponentSetSize (%u)! returning NULL!", getComponentName<T>(), entity, entityComponentSetSize);
            return NULL;
        }

        if (entity == NULL_ENTITY) {
            Log("ComponentPool(%s)::get Error: Attempted to get a component of a non-existent entity! Returning NULL. Entity: %u", getComponentName<T>(), entity);
            return NULL;
        }
        
        Uint32 componentIndex = entityComponentSet[entity];
        if (componentIndex == NULL_COMPONENTPOOL_INDEX) {
            /*
            // make a new component for the entity that is apparently missing a component
            Log("attempted access to component of entity (%d) that did not have a component of the type (%d)."
                "Adding component to entity.", entity, getID<T>());
            add(entity);
            return get(entity);
            */
            Log("ComponentPool(%s)::get : Attempted access to component of entity (%u) that did not have a component of the correct type."
                "Returning NULL", getComponentName<T>(), entity);
            return NULL;
        }
        if (componentIndex > size) {
            Log("ComponentPool(%s)::get : Error: componentIndex (%d) of entityComponentSet[%d] is greater than size: %d", getComponentName<T>(), componentIndex, entity, size);
            return NULL;
        }
        if (entity != componentOwners[componentIndex]) {
            Log("ComponentPool(%s)::get : Entity not listed as owner of component accessing component index %d! Entity: %d", getComponentName<T>(), componentIndex, entity);
        }
        return &components[componentIndex];
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        if (entityComponentSet[entity] != NULL_COMPONENTPOOL_INDEX) {
            LogError("ComponentPool(%s)::add : Entity %u already has this component! Aborting.", getComponentName<T>(), entity);
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
        if (entity == NULL_ENTITY) {
            LogError("ComponentPool(%s)::remove : Entity passed was NULL!", getComponentName<T>());
            return -1;
        }

        //Log("Removing component %d of entity %d", getID<T>(), entity);
        if (entityComponentSet[entity] == NULL_COMPONENTPOOL_INDEX || size == 0) {
            LogError("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %d, component index: %d, size: %d", getComponentName<T>(), entity, entityComponentSet[entity], size);
            return -1;
        }

        Uint32 lastComponentIndex = size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner == NULL_ENTITY) {
            LogError("Last component owner is NULL while removing entity %d!", entity);
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
                LogError("Component index (%u) of entity %u is greater than componentpool size %u!", entityComponentSet[entity], entity, size);
            }
            entityComponentSet[lastComponentOwner] = entityComponentSet[entity];
            componentOwners[entityComponentSet[entity]] = lastComponentOwner;
        }
        
        // update it to be empty
        entityComponentSet[entity] = NULL_COMPONENTPOOL_INDEX;
        componentOwners[lastComponentIndex] = NULL_ENTITY;
        size--;
        return 0;
    }

    void iterate(std::function<void(T*)> callback) {
        for (int c = size-1; c >= 0; c--) {
            callback(&components[c]);
        }
    }

private:
    void resize(Uint32 newSize) {
        if (newSize < size) {
            // bad things
            Log("Warning: component pool was called with a resize value too small to contain all entities.\n"
                "old size: %d, new size: %d. Aborting.", size, newSize);
            return;
        }

        T* newComponents = (T*)realloc(components, newSize * sizeof(T));
        if (!newComponents) {
            LogError("Failed to realloc component pool! Component id: %d", getID<T>());
            newComponents = (T*)malloc(newSize * sizeof(T));
            memcpy(newComponents, components, size * sizeof(T));
            free(components);
        }
        components = newComponents;
        
        EntityID* newOwners = (EntityID*)realloc(componentOwners, newSize * sizeof(EntityID));
        if (!newOwners) {
            //Log("Failed to realloc component pool owners");
            newOwners = (EntityID*)malloc(newSize * sizeof(EntityID));
            memcpy(newOwners, componentOwners, size * sizeof(EntityID));
            free(componentOwners);
        }
        componentOwners = newOwners;
        

        reserved = newSize;
    }
};

#endif