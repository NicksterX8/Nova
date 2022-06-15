#ifndef ENTITY_MANAGER_INCLUDED
#define ENTITY_MANAGER_INCLUDED

#include <stdlib.h>
#include <stddef.h>
#include <bitset>
#include <functional>
#include <vector>
#include <string>
#include "../constants.hpp"
#include "../NC/cpp-vectors.hpp"
#include "../Log.hpp"

#include "Entity.hpp"
#include "Component.hpp"
#include "ComponentPool.hpp"

#include "../ComponentMetadata/component.hpp"

typedef unsigned int Uint32;

using EntityCallback = std::function<void(_Entity)>;

class EntityManager {
    _Entity entities[MAX_ENTITIES];
    Uint32 indices[MAX_ENTITIES];
    ComponentFlags entityComponentFlags[MAX_ENTITIES];

    Uint32 liveEntities = 0;
    std::vector<_Entity> freeEntities = std::vector<_Entity>(MAX_ENTITIES);

    void* pools[NUM_COMPONENTS] = {NULL};
    Uint32 nComponents = 0;

    template<class T>
    int pool_remove_sig(EntityID entity, ComponentFlags signature) {
        if ((componentSignature<T>() & signature).any()) {
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

    /*
    int removeEntityComponents(EntityID entity, ComponentFlags signature) {
        int codes[] = { 0, ( (void)0, pool_remove_component<Components>(entity)) ... };
        int code = 0;
        for (size_t i = 0; i < sizeof(codes) / sizeof(int); i++) {
            code |= codes[i];
        }
        return code;
    }
    */

    template<class T>
    void pool_add_sig(EntityID entity, ComponentFlags signature) {
        if ((componentSignature<T>() & signature).any()) {
            ComponentPool<T>* pool = getPool<T>();
            pool->add(entity);
        }
    }

    template<class... Components>
    void addEntitySignature(EntityID entity, ComponentFlags newComponents) {
        int dummy[] = { 0, ( (void)pool_add_sig<Components>(entity, newComponents), 0) ... };
        (void)dummy;
    }

    template<class T>
    int pool_add_component(EntityID entity) {
        ComponentPool<T>* pool = getPool<T>();
        return pool->add(entity);
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
    ComponentPool<T>* getPool() const {
        return static_cast<ComponentPool<T>*>(pools[getID<T>()]);
    }
public:
    template<class T>
    void newComponent() {
        Uint32 id = getID<T>();
        assert(id == nComponents);
        // make a new pool
        ComponentPool<T>* pool = new ComponentPool<T>(100);
        pools[id] = pool;
        nComponents++;
        if (nComponents > NUM_COMPONENTS) {
            LogCritical("The number of components in the enity component system is greater than the constant NUM_COMPONENTS!"
                "number of components in system: %u, NUM_COMPONENTS: %d", nComponents, NUM_COMPONENTS);
        }
    }

    template<class... Components>
    void newComponents() {
        int dummy[] = {0, (newComponent<Components>(), 0) ...};
        (void)dummy;
    }

public:
    EntityManager();

    template<class... Components>
    void init() {
        newComponents<Components...>();
        
        // all entities are free be default
        for (Uint32 i = 0; i < MAX_ENTITIES; i++) {
            _Entity newEntity = {i, 0};
            freeEntities[(MAX_ENTITIES-1) - i] = newEntity;

            entities[i].id = NULL_ENTITY;
            indices[i] = NULL_ENTITY;

            entityComponentFlags[i] = ComponentFlags(false);
        }
    }

    void destroy();
    Uint32 numLiveEntities() const;
    bool EntityLives(_Entity entity) const;
    _Entity New();

    template<class T> 
    T* Get(_Entity entity) const {
        if (!EntityLives(entity)) {
            LogError("EntityManager::Get : Attempted to get a component of a non-living entity! Returning NULL. EntityID: %u. Version: %u", entity.id, entity.version);
            return NULL;
        }
        if (entityComponentFlags[entity.id][getID<T>()]) {
            return (getPool<T>())->get(entity.id);
        }
        else {
            LogError("Attempted to get a component of an entity that the entity does not have in its flags! Returning NULL.");
            return NULL;
        }
    }

    template<class... Components>
    int Add(_Entity entity) {
        if (!EntityLives(entity)) {
            LogError("EntityManager::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        ComponentFlags signature = componentSignature<Components...>();
        addEntityComponents<Components...>(entity.id);
        entityComponentFlags[entity.id] |= signature;
        return 0;
    }

    template<class T>
    int Add(_Entity entity, T startValue) {
        if (!EntityLives(entity)) {
            LogError("EntityManager::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        addEntityComponents<T>(entity.id);
        entityComponentFlags[entity.id] |= componentSignature<T>();
        *Get<T>(entity) = startValue;
        return 0;
    }

    template<class... Components>
    int AddSignature(_Entity entity, ComponentFlags signature) {
        if (!EntityLives(entity)) {
            LogError("EntityManager::Add : Attempted to add a component to a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }
        
        // just in case the same component is tried to be added twice
        ComponentFlags newComponents = (signature ^ entityComponentFlags[entity.id]) & signature;
        entityComponentFlags[entity.id] |= signature;
        addEntitySignature<Components...>(entity.id, newComponents);
        return 0;
    }

    template<class... Components>
    int Remove(_Entity entity) {
        if (!EntityLives(entity)) {
            LogError("ECS::Remove : Attempted to remove a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }
        
        int componentPoolReturnCode = removeEntityComponentsAll<Components...>(entity.id);
        // swap last entity with this entity for 100% entity packing in array
        Uint32 lastEntityIndex = liveEntities-1;
        _Entity lastEntity = entities[lastEntityIndex];
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
        entityComponentFlags[entity.id] = ComponentFlags(false); // set this to 0 just in case

        return componentPoolReturnCode;
    }

    template<class... Components>
    int RemoveComponents(_Entity entity) {
        if (!EntityLives(entity)) {
            LogError("EntityManager::Remove : Attempted to remove components from a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        int code = removeEntityComponents<Components...>(entity.id);
        entityComponentFlags[entity.id] ^= (componentSignature<Components...>() & entityComponentFlags[entity.id]);

        return code;
    }

    template<class T>
    Uint32 componentPoolSize() {
        return getPool<T>()->getSize();
    }

    _Entity getEntityByIndex(Uint32 entityIndex);

    ComponentFlags entityComponents(EntityID entityID) const;

    // garbo 

    void iterateEntities(std::function<bool(ComponentFlags signature)> query, EntityCallback callback) {
        // iterate in reverse to maybe make removing safeish??
        for (Uint32 e = 0; e < liveEntities; e++) {
            _Entity entity = entities[e];
            ComponentFlags signature = entityComponentFlags[entity.id];

            if (query(signature)) {
                callback(entity);  
            }
        }
    }

    template<class First, class... Components>
    void iterateEntities(EntityCallback callback) {
        iterateEntities(componentSignature<First, Components...>(), callback);
    }
    
    void iterateEntities(EntityCallback& callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            callback(entities[e]);
        }
    }
    
    void iterateEntities(ComponentFlags componentSignature, EntityCallback callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            // entity has all bits 1 that the signature has 1
            ComponentFlags both = entityComponentFlags[entities[e].id] & componentSignature;
            
            if (both == componentSignature) {
                // entity has all components
                callback(entities[e]);
            }
        }
    }    

    template<class T>
    void iterateComponents(std::function<void(T*)> callback) {
        getPool<T>()->iterate(callback);
    }

    template<class T, class... EntityComponents> 
    T* Get(_EntityType<EntityComponents...> entity) {
        //constexpr ComponentFlags signature = constComponentSignature<EntityComponents...>();
        //constexpr bool hasit = signature[getID<T>()];
        static_assert(componentInComponents<T, EntityComponents...>(),
            "Entity must have correct type to get a component!");

        return Get<T>((Entity)entity);
    }

    template<class T, class... EntityComponents>
    _EntityType<T, EntityComponents...>& AddT(_EntityType<EntityComponents...> entity) {
        Add<T>(entity);
        return entity.template cast<T, EntityComponents...>();
    }

    template<class T, class... EntityComponents>
    _EntityType<T, EntityComponents...>& AddT(_EntityType<EntityComponents...> entity, T startValue) {
        Add<T>(entity, startValue);
        return entity.template cast<T, EntityComponents...>();
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
};

#endif