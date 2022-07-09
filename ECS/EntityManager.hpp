#ifndef ENTITY_MANAGER_INCLUDED
#define ENTITY_MANAGER_INCLUDED

#include <stdlib.h>
#include <assert.h>
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
public:
    EntityBase entities[MAX_ENTITIES];
    Uint32 indices[MAX_ENTITIES];
    ComponentFlags entityComponentFlags[MAX_ENTITIES];

    Uint32 liveEntities = 0;
    std::vector<EntityBase> freeEntities = std::vector<EntityBase>(MAX_ENTITIES-1);

    ComponentPool* pools[NUM_COMPONENTS] = {NULL};
    Uint32 nComponents = 0;

    /*
    template<class T>
    int pool_remove_sig(EntityID entity, ComponentFlags signature) {
        if (sizeof(T) == 0) return 1;
        if (signature[getID<T>()]) {
            ComponentPool<T>* pool = getPool<T>();
            return pool->remove(entity);
        }
        return 0;
    }

    template<class T>
    int pool_remove_component(EntityID entity) {
        if (sizeof(T) == 0) return 1;
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
    void pool_add_sig(EntityID entity, ComponentFlags signature) {
        if (sizeof(T) == 0) return;
        if (signature[getID<T>()]) {
            ComponentPool<T>* pool = getPool<T>();
            pool->add(entity);
        }
    }

    template<class... Components>
    void addEntitySignature(EntityID entity, ComponentFlags newComponents) {
        int dummy[] = { 0, ( (void)pool_add_sig<Components>(entity, newComponents), 0) ... };
        (void)dummy;
    }
    */

    template<class T>
    int pool_add_component(EntityID entity) {
        if (sizeof(T) == 0) return 1;
        ComponentPool* pool = getPool<T>();
        return pool->add(entity);
    }

    template<class... Components>
    void addEntityComponents(EntityID entity) {
        int dummy[] = { 0, ( (void)pool_add_component<Components>(entity), 0) ... };
        (void)dummy;
    }

    void destroyPools() {
        for (ComponentID id = 0; id < nComponents; id++) {
            pools[id]->destroy();
        }
    }

    template<class T>
    ComponentPool* getPool() const {
        return pools[getID<T>()];
    }
public:
    template<class T>
    void newComponent() {
        Uint32 id = getID<T>();
        assert(id == nComponents);
        // make a new pool
        ComponentPool* pool = NULL;
        if (sizeof(T) != 0)
            pool = new ComponentPool(getID<T>(), sizeof(T), 100);
        pools[id] = pool;
        nComponents++;
        if (nComponents > NUM_COMPONENTS) {
            Log.Critical("The number of components in the enity component system is greater than the constant NUM_COMPONENTS!"
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
        
        // all entities are free by default
        for (Uint32 i = 0; i < MAX_ENTITIES; i++) {
            entities[i] = NullEntity;
            indices[i] = NULL_ENTITY_ID;

            entityComponentFlags[i] = ComponentFlags(false);
        }

        for (Uint32 i = MAX_ENTITIES-2;; i--) {
            EntityBase newEntity = EntityBase(MAX_ENTITIES - i - 2, 0);
            freeEntities[i] = newEntity;
            if (i == 0) break;
        }
    }

    void destroy();
    Uint32 numLiveEntities() const;
    bool EntityExists(_Entity entity) const;
    template<class... Cs>
    inline bool EntityHas(_Entity entity) const {
        constexpr ComponentFlags signature = componentSignature<Cs...>();
        return (entityComponents(entity.id) & signature) == signature;
    }
    _Entity New();

    template<class T> 
    T* Get(_Entity entity) const {
        if (sizeof(T) == 0) {
            return NULL;
        }

        if (entity.id < NULL_ENTITY_ID && entity.version < NULL_ENTITY_VERSION) {
            if (entities[indices[entity.id]].version == entity.version) {
                ComponentFlags entityFlags = entityComponentFlags[entity.id];
                if ((entityFlags & componentSignature<T>()).any()) {
                    return static_cast<T*>(getPool<T>()->get(entity.id));
                }
            }
        }

        Log.Error("Attempted to get unowned component \"%s\" of an entity! Returning NULL. Entity: %s", componentNames[getID<T>()], entity.DebugStr().c_str());
        return NULL;
    }

    void* Get(ComponentID componentID, _Entity entity) const {
        ComponentPool* pool = pools[componentID];
        if (pool) {
            if (entity.id < NULL_ENTITY_ID && entity.version < NULL_ENTITY_VERSION) {
                if (entities[indices[entity.id]].version == entity.version) {
                    ComponentFlags entityFlags = entityComponentFlags[entity.id];
                    if (entityFlags[componentID]) {
                        return pool->get(entity.id);
                    }
                }
            }
        }

        Log.Error("Failed to get component \"%s\" of an entity! Returning NULL. Entity: %s", componentNames[componentID], entity.DebugStr().c_str());
        return NULL;
    }

    template<class... Components>
    int Add(_Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::Add : Attempted to add components to a non-living entity! Returning -1. Entity: %s, Components: %s", entity.DebugStr().c_str(), getComponentNameList<Components...>().c_str());
            return -1;
        }

        ComponentFlags signature = componentSignature<Components...>();
        ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
        // start at one to account for dummy value
        for (Uint32 i = 1; i < sizeof(ids) / sizeof(ComponentID); i++) {
            ComponentID id = ids[i];
            pools[id]->add(entity.id);
        }
        entityComponentFlags[entity.id] |= signature;
        return 0;
    }

    template<class T>
    int Add(_Entity entity, const T& startValue) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::Add %s : Attempted to add a component to a non-living entity! Returning -1. Entity: %s", getComponentName<T>(), entity.DebugStr().c_str());
            return -1;
        }

        entityComponentFlags[entity.id] |= componentSignature<T>();

        ComponentPool* pool = getPool<T>();
        pool->add(entity.id);
        *Get<T>(entity) = startValue;
        return 0;
    }

    int AddSignature(_Entity entity, ComponentFlags signature) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::AddSignature : Attempted to add components to a non-existent entity! Returning -1. Entity: %s", entity.DebugStr().c_str());
            return -1;
        }
        
        // just in case the same component is tried to be added twice
        ComponentFlags newComponents = (signature ^ entityComponentFlags[entity.id]) & signature;
        entityComponentFlags[entity.id] |= signature;
        int code = 0;
        for (ComponentID id = 0; id < nComponents; id++) {
            if (signature[id])
                code |= pools[id]->add(entity.id);
        }
        return code;
    }

    int Destroy(_Entity entity) {
        ComponentFlags entityComponents = entityComponentFlags[entity.id];

        if (!EntityExists(entity)) {
            Log.Error("ECS::Remove %s : Attempted to remove a non-existent entity! Returning -1. Entity: %s", entity.DebugStr().c_str());
            return -1;
        }

        int code = 0;
        for (Uint32 id = 0; id < nComponents; id++) {
            if (entityComponents[id])
                code |= pools[id]->remove(entity.id);
        }

        // swap last entity with this entity for 100% entity packing in array
        Uint32 lastEntityIndex = liveEntities-1;
        EntityBase lastEntity = entities[lastEntityIndex];
        
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
        entities[liveEntities].id = NULL_ENTITY_ID;
        indices[entity.id] = NULL_ENTITY_ID;
        entityComponentFlags[entity.id] = ComponentFlags(false); // set this to 0 just in case

        return code;
    }

    int RemoveComponents(_Entity entity, ComponentID* componentIDs, Uint32 numComponentIDs) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::RemoveComponents : Attempted to remove components from a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        int code = 0;
        for (Uint32 i = 0; i < numComponentIDs; i++) {
            ComponentID id = componentIDs[i];
            if (entityComponentFlags[entity.id][id]) {
                code |= pools[id]->remove(entity.id);
                entityComponentFlags[entity.id].set(id, 0);
            }
        }
        return code;
    }

    template<class... Components>
    int RemoveComponents(_Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::RemoveComponents : Attempted to remove components from a non-living entity! Returning -1. EntityID: %u. Version: %u", entity.id, entity.version);
            return -1;
        }

        auto componentIDs = getComponentIDs<Components...>();

        int code = 0;
        for (Uint32 i = 0; i < componentIDs.size(); i++) {
            ComponentID id = componentIDs[i];
            if (entityComponentFlags[entity.id][id]) {
                code |= pools[id]->remove(entity.id);
            }
        }
        entityComponentFlags[entity.id] |= componentSignature<Components...>();
        return code;
    }

    template<class T>
    Uint32 componentPoolSize() const {
        return getPool<T>()->getSize();
    }

    _Entity getEntityByIndex(Uint32 entityIndex);

    ComponentFlags entityComponents(EntityID entityID) const;

    // garbo 

    void iterateEntities(std::function<bool(ComponentFlags signature)> query, EntityCallback callback) {
        // iterate in reverse to maybe make removing safeish??
        for (Uint32 e = 0; e < liveEntities; e++) {
            EntityBase entity = entities[e];
            ComponentFlags signature = entityComponentFlags[entity.id];

            if (query(signature)) {
                callback(baseToEntity(entity));  
            }
        }
    }

    template<class First, class... Components>
    void iterateEntities(EntityCallback callback) {
        iterateEntities(componentSignature<First, Components...>(), callback);
    }
    
    void iterateEntities(EntityCallback& callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            callback(baseToEntity(entities[e]));
        }
    }
    
    void iterateEntities(ComponentFlags componentSignature, EntityCallback callback) {
        for (Uint32 e = 0; e < liveEntities; e++) {
            // entity has all bits 1 that the signature has 1
            ComponentFlags both = entityComponentFlags[entities[e].id] & componentSignature;
            
            if (both == componentSignature) {
                // entity has all components
                callback(baseToEntity(entities[e]));
            }
        }
    }    

    template<class T>
    void iterateComponents(std::function<void(T&)> callback) const {
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