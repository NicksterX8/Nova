#ifndef ECS_INCLUDED
#define ECS_INCLUDED

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stddef.h>
#include <bitset>
#include "Components.hpp"
#include <functional>
#include <vector>
#include "constants.hpp"
#include "NC/cpp-vectors.hpp"
#include "Textures.hpp"
#include "Log.hpp"

#define MAX_ENTITIES 50000
#define NULL_ENTITY (MAX_ENTITIES-1)
#define MAX_COMPONENTS 32
#define NUM_COMPONENTS 14

typedef Uint32 EntityID;
typedef Uint32 EntityVersion;
typedef std::bitset<NUM_COMPONENTS> ComponentFlags;

struct Entity {
    EntityID id;
    EntityVersion version;

    bool operator==(Entity rhs) {
        return (id == rhs.id && version == rhs.version);
    }
};

template<typename T>
Uint32 componentID() {
    static Uint32 id = componentCount++;
    return id;
}

template <class T>
Uint32 singleComponentSignature() {
    return (1 << componentID<T>());
}

template<typename... Components>
Uint32 componentSignature() {
    // void 0 to remove unused result warning
    Uint32 signatures[] = { 0, ( (void)0, singleComponentSignature<Components>()) ... };
    // sum component signatures
    Uint32 result = 0;
    for (unsigned long i = 0; i < sizeof(signatures) / sizeof(Uint32); i++) {
        result |= signatures[i];
    }
    return result;
}

template<class T>
class ComponentPool {
    T* components = NULL; // array of components
    Uint32 size = 0; // how many used components exist in the components array
    Uint32* entityComponentSet = NULL; // array indexable by entity id to get the index of the component owned by that entity
    Uint32 entityComponentSetSize = 0;

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

    T* get(EntityID entity) {
        // this check is removed to increase performance
        
        if (entity > entityComponentSetSize) {
            Log("ComponentPool::get was called with an entity higher than entityComponentSetSize! returning NULL!");
            return NULL;
        }

        if (entity == NULL_ENTITY) {
            Log("ComponentPool::get Error: Attempted to get a component of a non-existent entity! Returning NULL. Entity: %u", entity);
            return NULL;
        }
        
        Uint32 componentIndex = entityComponentSet[entity];
        if (componentIndex == NULL_COMPONENTPOOL_INDEX) {
            /*
            // make a new component for the entity that is apparently missing a component
            Log("attempted access to component of entity (%d) that did not have a component of the type (%d)."
                "Adding component to entity.", entity, componentID<T>());
            add(entity);
            return get(entity);
            */
            Log("attempted access to component of entity (%d) that did not have a component of the type (%d)."
                "Returning NULL", entity, componentID<T>());
            return NULL;
        }
        if (componentIndex > size) {
            Log("ComponentPool::get Error: componentIndex (%d) of entityComponentSet[%d] is greater than size: %d", componentIndex, entity, size);
            return NULL;
        }
        if (entity != componentOwners[componentIndex]) {
            Log("Entity not listed as owner of component accessing component index %d! Entity: %d", componentIndex, entity);
        }
        return &components[componentIndex];
    }

    void add(EntityID entity) {
        //Log("Adding component %d to entity %d", componentID<T>(), entity);
        if (size >= reserved) {
            // double in size every time it runs out of space
            // Log("resizing pool, old size: %d, new size: %d, reserved: %d", size, reserved * 2, reserved);
            resize(reserved * 2);
        }
        entityComponentSet[entity] = size;
        componentOwners[size] = entity;
        size++;
    }

    int remove(EntityID entity) {
        if (entity == NULL_ENTITY) {
            LogError("Entity passed to ComponentPool::remove was NULL! component id: %d", componentID<T>());
            return -1;
        }

        //Log("Removing component %d of entity %d", componentID<T>(), entity);
        if (entityComponentSet[entity] == NULL_COMPONENTPOOL_INDEX || size == 0) {
            LogError("Failed to remove entity from component pool. Entity: %d, component index: %d, size: %d, component id: %d", entity, entityComponentSet[entity], size, componentID<T>());
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
            LogError("Failed to realloc component pool! Component id: %d", componentID<T>());
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

struct EntityData {
    Uint32 componentFlags;
    Entity entity;
};

namespace Tests {
    class ECSTest;
}

class ECS {
    std::vector<void*> pools{MAX_COMPONENTS};
    Entity entities[MAX_ENTITIES];
    Uint32 entityComponentFlags[MAX_ENTITIES];
    Uint32 liveEntities = 0;
    std::vector<Entity> freeEntities = std::vector<Entity>(MAX_ENTITIES);
    Uint32 nComponents = 0;

    Uint32 indices[MAX_ENTITIES];

    std::vector<Entity> entitiesToBeRemoved;

    template<class T>
    int pool_remove_sig(EntityID entity, Uint32 signature) {
        if (componentSignature<T>() & signature) {
            ComponentPool<T>* pool = getPool<T>();
            return pool->remove(entity);
        }
        return 0;
    }

    template<class T>
    int pool_remove_component(EntityID entity) {
        ComponentPool<T>* pool = getPool<T>();
        return pool->remove(entity);
    }

    template<class... Components>
    int removeEntityComponentsAll(EntityID entity) {
        int codes[] = { 0, ( (void)0, pool_remove_sig<Components>(entity, entityComponentFlags[entity])) ... };
        int code = 0;
        for (size_t i = 0; i < sizeof(codes) / sizeof(int); i++) {
            code |= codes[i];
        }
        return code;
    }

    template<class... Components>
    int removeEntityComponents(EntityID entity) {
        int codes[] = { 0, ( (void)0, pool_remove_component<Components>(entity)) ... };
        int code = 0;
        for (size_t i = 0; i < sizeof(codes) / sizeof(int); i++) {
            code |= codes[i];
        }
        return code;
    }

    template<class T>
    void pool_add_sig(EntityID entity, Uint32 signature) {
        if (componentSignature<T>() & signature) {
            ComponentPool<T>* pool = getPool<T>();
            pool->add(entity);
        }
    }

    template<class... Components>
    void addEntitySignature(EntityID entity, Uint32 newComponents) {
        int dummy[] = { 0, ( (void)pool_add_sig<Components>(entity, newComponents), 0) ... };
        (void)dummy;
    }

    template<class T>
    void pool_add_component(EntityID entity) {
        ComponentPool<T>* pool = getPool<T>();
        pool->add(entity);
    }

    template<class... Components>
    void addEntityComponents(EntityID entity) {
        int dummy[] = { 0, ( (void)pool_add_component<Components>(entity), 0) ... };
        (void)dummy;
    }

    template<class T>
    void pool_destroy() {
        ComponentPool<T>* pool = getPool<T>();
        pool->destroy();
    }

    template<class... Components>
    void _destroyPools() {
        int dummy[] = { 0, ( (void)pool_destroy<Components>(), 0) ... };
        (void)dummy;
    }

    void destroyPools() {
        _destroyPools<COMPONENTS>();
    }

    template<class T>
    ComponentPool<T>* getPool() {
        return static_cast<ComponentPool<T>*>(pools[componentID<T>()]);
    }

    template<class T>
    void newComponent() {
        Uint32 id = componentID<T>();
        assert(id == nComponents);
        // make a new pool
        ComponentPool<T>* pool = new ComponentPool<T>(100);
        pools[id] = pool;
        nComponents++;
        if (nComponents > MAX_COMPONENTS) {
            LogCritical("The number of components in the enity component system is greater than the maximum!"
                "number of components: %d, maximum components: %d", nComponents, MAX_COMPONENTS);
        }
    }

    template<class... Components>
    void newComponents() {
        int dummy[] = {0, (newComponent<Components>(), 0) ...};
        (void)dummy;
    }

public:
friend class Tests::ECSTest;
    ECS() {
        newComponents<COMPONENTS>();
        // all entities are free be default
        for (Uint32 i = 0; i < MAX_ENTITIES; i++) {
            Entity newEntity = {i, 0};
            freeEntities[(MAX_ENTITIES-1) - i] = newEntity;

            entities[i].id = NULL_ENTITY;
            indices[i] = NULL_ENTITY;
        }

    }

    void destroy() {
        destroyPools();
    }

    Uint32 numLiveEntities() {
        return liveEntities;
    }

    bool EntityLives(Entity entity) {
        if (entity.id != NULL_ENTITY) {
            Uint32 index = indices[entity.id];
            if (index != NULL_ENTITY) {
                return (entities[index].version == entity.version);
            }
        }
        
        return false;
    }

    Entity New() {
        if (liveEntities < MAX_ENTITIES - 1 && freeEntities.size() > 0) {
            // get top entity from free entity stack
            Entity entity = freeEntities.back();
            if (entity.id == NULL_ENTITY) {
                LogError("Entity from freeEntities was NULL! EntityID: %d", entity.id);
            }
            freeEntities.pop_back();

            // add free entity to end of entities array
            Uint32 index = liveEntities;
            entities[index] = entity;
            indices[entity.id] = index; // update the indices array to tell where the entity is stored
            entityComponentFlags[entity.id] = 0; // entity starts with no components
            liveEntities++;

            return entity;
        }
        LogError("Ran out of entity space in ECS! Live entities: %u, freeEntities size: %lu. Aborting with return NULL_ENTITY.\n", liveEntities, freeEntities.size());
        return {NULL_ENTITY, 0};
    }

    template<class T> 
    T* Get(Entity entity) {
        if (!EntityLives(entity)) {
            LogError("ECS::Get : Attempted to get a component of a non-living entity! Returning NULL. EntityID: %u. Version: %u", entity.id, entity.version);
            return NULL;
        }
        if (componentSignature<T>() & entityComponentFlags[entity.id]) {
            return (getPool<T>())->get(entity.id);
        }
        else {
            LogError("Attempted to get a component of an entity that the entity does not have in its flags! Returning NULL.");
            return NULL;
        }
    }

    template<class... Components>
    int Add(Entity entity) {
        if (!EntityLives(entity)) {
            LogError("ECS::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        Uint32 signature = componentSignature<Components...>();
        entityComponentFlags[entity.id] |= signature;
        addEntityComponents<Components...>(entity.id);
        return 0;
    }

    template<class T>
    int Add(Entity entity, T startValue) {
        if (!EntityLives(entity)) {
            LogError("ECS::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        entityComponentFlags[entity.id] |= componentSignature<T>();
        addEntityComponents<T>(entity.id);
        *Get<T>(entity) = startValue;
        return 0;
    }

    int Add(Entity entity, Uint32 signature) {
        if (!EntityLives(entity)) {
            LogError("ECS::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }
        
        // just in case the same component is tried to be added twice
        Uint32 newComponents = (signature ^ entityComponentFlags[entity.id]) & signature;
        entityComponentFlags[entity.id] |= signature;
        addEntitySignature<COMPONENTS>(entity.id, newComponents);
        return 0;
    }

    int Remove(Entity entity) {
        if (!EntityLives(entity)) {
            LogError("ECS::Remove : Attempted to remove a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }
        
        int componentPoolReturnCode = removeEntityComponentsAll<COMPONENTS>(entity.id);
        // swap last entity with this entity for 100% entity packing in array
        Uint32 lastEntityIndex = liveEntities-1;
        Entity lastEntity = entities[lastEntityIndex];
        if (entity == lastEntity) {
            // I dont think it matters
        }

        Uint32 entityIndex = indices[entity.id]; // the index of the entity being removed
        // move last entity to removed entity position in array
        entities[entityIndex] = lastEntity;
        // tell indices where the (formerly) last entity now is
        indices[lastEntity.id] = entityIndex;

        // Version goes up one everytime it's removed
        entity.version += 1;

        freeEntities.push_back(entity);
        liveEntities--;

        // set now unused last entity position data to null
        entities[liveEntities].id = NULL_ENTITY;
        indices[entity.id] = NULL_ENTITY;
        entityComponentFlags[entity.id] = 0; // set this to 0 just in case
        return componentPoolReturnCode;
    }

    template<class... Components>
    int RemoveComponents(Entity entity) {
        if (!EntityLives(entity)) {
            LogError("ECS::Remove : Attempted to remove a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        int code = removeEntityComponents<Components...>(entity.id);
        entityComponentFlags[entity.id] ^= (componentSignature<Components...>() & entityComponentFlags[entity.id]);
        return code;
    }

    /*
    * Scheduling removes isn't necessary at this time, but it might be needed in the future
    * OK it is useful for actually plowing through components instead of stopping and removing entities,
    * you can just schedule them to go all at once
    */
    
    void ScheduleRemove(Entity entity) {
        entitiesToBeRemoved.push_back(entity);
    }

    int DoScheduledRemoves() {
        if (entitiesToBeRemoved.size() > 0) {
            //Log("size of scheduled removes: %d", entitiesToBeRemoved.size());
        }
        int code = 0;
        for (Entity entity : entitiesToBeRemoved) {
            code |= Remove(entity);
        }

        entitiesToBeRemoved.clear();
        return code;
    }

    template<class T>
    Uint32 componentPoolSize() {
        return getPool<T>()->getSize();
    }

    Uint32 entityComponents(EntityID entityID) {
        return entityComponentFlags[entityID];
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     

    using EntityCallback = std::function<void(ECS*, Entity)>;
    // template<class... Components>
    // using ComponentCallback = std::function<void(Components...)>();

    void iterateEntities(std::function<bool(Uint32 signature)> query, EntityCallback callback) {
        // iterate in reverse to maybe make removing safeish??
        for (Uint32 e = 0; e < liveEntities; e++) {
            Entity entity = entities[e];
            Uint32 signature = entityComponentFlags[entity.id];

            if (query(signature)) {
                callback(this, entity);  
            }
        }
    }


    template<class First, class... Components>
    void iterateEntities(EntityCallback callback) {
        iterateEntities(componentSignature<First, Components...>(), callback);
    }
    
    void iterateEntities(EntityCallback& callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            callback(this, entities[e]);
        }
    }
    
    void iterateEntities(Uint32 componentSignature, EntityCallback callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            // entity has all bits 1 that the signature has 1
            if ((entityComponentFlags[entities[e].id] & componentSignature) == componentSignature) {
                // entity has all components
                callback(this, entities[e]);
            }
        }
    }

    template<class C1>
    void iterateComponents(std::function<void(C1*)> callback) {
        Uint32 signature = componentSignature<C1>();
        for (Uint32 e = 0; e < liveEntities; e++) {
            Entity entity = entities[e];
            if ((entityComponentFlags[entity.id] & signature) == signature) {
                // entity has the, do the thing
                // NEED TO STATIC CAST BEFORE ACCESSING ARRAY!!!
                C1* component = Get<C1>(entity);
                callback(component);
            }
        }
    }
    template<class C1, class C2>
    void iterateComponents(std::function<void(C1*, C2*)> callback) {
        Uint32 signature = componentSignature<C1, C2>();
        for (Uint32 e = 0; e < liveEntities; e++) {
            Entity entity = entities[e];
            // entity has all bits 1 that the signature has 1
            if ((entityComponentFlags[entity.id] & signature) == signature) {
                // entity has both components, do the thing
                // NEED TO STATIC CAST BEFORE ACCESSING ARRAY!!!
                callback(Get<C1>(entity), Get<C2>(entity));
            }
        }
    }
    template<class C1, class C2, class C3>
    void iterateComponents(std::function<void(C1*, C2*, C3*)> callback) {
        Uint32 signature = componentSignature<C1, C2, C3>();
        for (Uint32 e = 0; e < liveEntities; e++) {
            Entity entity = entities[e];
            // entity has all bits 1 that the signature has 1
            if ((entityComponentFlags[entity.id] & signature) == signature) {
                // entity has both components, do the thing
                // NEED TO STATIC CAST BEFORE ACCESSING ARRAY!!!
                callback(Get<C1>(entity), Get<C2>(entity), Get<C3>(entity));
            }
        }
    }
    template<class C1, class C2, class C3, class C4>
    void iterateComponents(std::function<void(C1*, C2*, C3*, C4*)> callback) {
        constexpr Uint32 signature = componentSignature<C1, C2, C3, C4>();
        for (Uint32 e = 0; e < liveEntities; e++) {
            Entity entity = entities[e];
            // entity has all bits 1 that the signature has 1
            if ((entityComponentFlags[entity.id] & signature) == signature) {
                // entity has both components, do the thing
                callback(Get<C1>(entity), Get<C2>(entity), Get<C3>(entity), Get<C4>(entity));
            }
        }
    }
};

/*
class EntitySystem {
    Uint32 componentSignature;
public:
    int order;

    EntitySystem(Uint32 signature) {
        componentSignature = signature;
    }

    Uint32 signature() {
        return componentSignature;
    }

    virtual void update(ECS* ecs, Entity entity) = 0;
};

class GrowthSystem : EntitySystem {
public:    
    GrowthSystem() : EntitySystem(componentSignature<GrowthComponent>()) {
       
    }

    void update(ECS* ecs, Entity entity) {
        
    }
};

class EntitySystemManager {
    std::vector<EntitySystem> systems;

    void update(ECS* ecs) {
        for (size_t i = 0; i < systems.size(); i++) {
            ecs->iterateEntities(systems[i].signature(), systems[i].update);
        }
    }
};
*/

#endif