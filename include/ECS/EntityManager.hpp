#ifndef ENTITY_MANAGER_INCLUDED
#define ENTITY_MANAGER_INCLUDED

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <stddef.h>
#include <functional>
#include <string>
#include "constants.hpp"
#include "utils/vectors.hpp"
#include "utils/Log.hpp"
#include "utils/common-macros.hpp"

#include "Entity.hpp"
#include "Component.hpp"
#include "ComponentPool.hpp"
#include "My/Vec.hpp"
#include "memory.hpp"


typedef uint32_t Uint32;

namespace ECS {

using EntityCallback = std::function<void(Entity)>;

struct EntityData {
    static constexpr Sint32 NullIndex = -1;

    EntityVersion version; // The current entity version
    Sint32 index; // Where in the entities array the entity is stored
    ComponentFlags flags; // The entity's component signature
};

struct EntityManager {
    Entity *entities;
    EntityData *entityDataList; // The list of data corresponding to an entity ID, indexed by the ID
    My::Vec<EntityID> freeEntities; // A stack of free entity ids

    Sint32 entityCount = 0; // The number of alive (or existent) entities

    // TODO: allocate pools in an array, find a solution to the optional pool problem.
    ComponentPool* pools = NULL; // a list of component pool pointers
    Sint32 nComponents = 0; // The number of unique component types, the size of the pools array
    ComponentID highestComponentID = 0; // The highest id of all the components used in the entity manager. Should be equal to nComponents most of the time


    /* Manager Methods */


    /*
    * @return The component pool corresponding to the component template parameter. May be null if the size of the component is 0 or for other reasons.
    */
    template<class T>
    inline ComponentPool* getPool() const {
        return getPool(getID<T>());
    }

    /*
    * The id passed should not be higher than (or equal to) the number of components.
    * @return The component pool corresponding to the component id. May be null if the size of the component is 0 or for other reasons.
    */
    inline ComponentPool* getPool(ComponentID id) const {
        if (id > highestComponentID) {
            LogError("EntityManager::getPool id passed is too high, could not get component pool!");
            return NULL;
        }
        
        ComponentPool* pool = &pools[id];
        if (pool->id != id) {
            // pool doesnt exist
            return nullptr;
        }
        return pool;
    }

    template<class... Components>
    static EntityManager Init() {
        EntityManager self;
        constexpr auto NumComponents = sizeof...(Components);
        self.nComponents = NumComponents;
    
        // initialize entity lists

        self.freeEntities = My::Vec<EntityID>::WithCapacity(MAX_ENTITIES-1);
        self.entities = (Entity*)malloc(MAX_ENTITIES * sizeof(Entity));
        self.entityDataList = (EntityData*)malloc(MAX_ENTITIES * sizeof(EntityData));

        // set all entities to null at start just in case, even though these should be overwritten before they're read.
        std::fill(self.entities, self.entities + MAX_ENTITIES, NullEntity);
        // the version must be set, as it is used when creating new entities.
        // the flags will be set to 0 again so it's a redundant safety measure.
        // the index is required to show that the entities are not in existence.
        for (EntityID i = 0; i < MAX_ENTITIES; i++) {
            static_assert(NULL_ENTITY_VERSION == 0, "starting entity version should be 1");
            entityDataList[i] = {
                .version = 1,
                .flags = ComponentFlags(0),
                .index = NULL_ENTITY_ID
            };
        }

        // all entities are free by default.
        // stop one shorter than the others to avoid making NULL_ENTITY_ID a free entity.
        freeEntities.size = MAX_ENTITIES-1;
        for (EntityID i = 0; i < NULL_ENTITY_ID; i++) {
            // set free entities in reverse order so that when new entities are popped off the back the ids will go from lowest to highest,
            // starting at 0, ending at NULL_ENTITY_ID-1
            freeEntities[(NULL_ENTITY_ID-1)-i] = i;
        }
        
        // initialize component pools

        self.pools = Alloc<ComponentPool>(NumComponents);
        for (ComponentID i = 0; i < NumComponents; i++) {
            self.pools[i].id = -1;
        }
        // make new pool for each component type
        FOR_EACH_VAR_TYPE(([&self](ComponentID id, size_t componentSize){
            self.pools[id] = ComponentPool(id, componentSize);
            self.highestComponentID = MAX(id, self.highestComponentID);
        })(getID<Components>(), sizeof(Components)));
    }

    // destroy the entity manager and deallocate everything
    void destroy();

    inline Sint32 getComponentCount() const {
        return nComponents;
    }

    inline Sint32 getEntityCount() const {
        return entityCount;
    }


    /* Component Methods */


    /* Get the name of the component type */
    template<class T>
    inline const char* getComponentName() const {
        return getComponentName(getID<T>());
    }

    /* Get the name of the component type corresponding to the id */
    const char* getComponentName(ComponentID id) const {
        const ComponentPool* pool = getPool(id);
        if (pool) return pool->name;
        return NULL;
    }

    /* Get the size of the component pool for the type */
    template<class T>
    Uint32 getComponentPoolSize() const {
        const ComponentPool* pool = getPool<T>();
        if (pool) return pool->size;
        return 0;
    }

    Entity getEntityByIndex(Uint32 entityIndex) const;


    /* Entity Methods */


    Entity New();

    Entity* New(int count, Entity* out);

    void Destroy(Entity entity);
    void Destroy(int count, const Entity* entities);

    ComponentFlags EntitySignature(EntityID entityID) const;

    template<class... Cs>
    inline bool EntityHas(Entity entity) const {
        constexpr ComponentFlags signature = componentSignature<Cs...>();
        return EntitySignature(entity.id).hasAll(signature);
    }

    inline bool EntityExists(Entity entity) const {
        return (entity.id < NULL_ENTITY_ID && entity.version >= entityDataList[entity.id].version);
    }

    // Get a component of the entity. May be NULL if the entity does not have the component
    template<class T> 
    T* Get(Entity entity) const {
        if (sizeof(T) == 0) return NULL;

        ComponentPool* pool = getPool<T>();
        if (entity.id < NULL_ENTITY_ID) {
            EntityData* entityData = &entityDataList[entity.id];
            if (entity.version >= entityData->version && entityData->flags.getComponent<T>())
                return static_cast<T*>(pool->get(entity.id));
        }

        LogError("Attempted to get unowned component \"%s\" of an entity! Returning NULL. Entity: %s", getComponentName<T>(), entity.DebugStr());
        return NULL;
    }

    // Get a component of the entity. May be NULL if the entity does not have the component
    void* Get(ComponentID componentID, Entity entity) const {
        ComponentPool* pool = getPool(componentID);
        if (EntityExists(entity) && pool)
            return pool->get(entity.id);

        LogError("Failed to get component \"%s\" of an entity! Returning NULL. Entity: %s", getComponentName(componentID), entity.DebugStr());
        return NULL;
    }

    template<class... Components>
    int Add(Entity entity) {
        if (!EntityExists(entity)) {
            LogError("Attempted to add components to a non-living entity! Entity: %s.", entity.DebugStr());
            return -1;
        }

        constexpr auto ids = getComponentIDs<Components...>();
        // start at one to account for dummy value
        int code = 0;
        for (Uint32 i = 0; i < sizeof...(Components); i++) {
            ComponentPool* pool = getPool(ids[i]);
            if (pool)
                code |= pool->add(entity.id);
        }
        entityDataList[entity.id].flags |= componentSignature<Components...>();
        return code;
    }

    int Add(ComponentID id, Entity entity) {
        if (EntityExists(entity)) {
            auto pool = getPool(id);
            if (pool) {
                entityDataList[entity.id].flags |= (1U << id);
                return pool->add(entity.id);
            }
            LogError("Attempted to add component %s to a non-living entity! Entity: %s", getComponentName(id), entity.DebugStr());
            return -1;
        }
        return -1;
    }

    int AddSignature(Entity entity, ComponentFlags signature) {
        if (!EntityExists(entity)) {
            LogError("Attempted to add components to a non-existent entity! Entity: %s", entity.DebugStr());
            return -1;
        }
        
        // just in case the same component is tried to be added twice
        ComponentFlags* components = &entityDataList[entity.id].flags;
        *components |= signature;
        int code = 0;
        for (ComponentID id = 0; id < nComponents; id++) {
            if (signature[id])
                code |= pools[id].add(entity.id);
        }
        signature.forEachSet([&](auto id){
            code |= pools[id].add(entity.id);
        });
        /*
        for (int i = 0; i < signature.nInts; i++) {
            uint64_t bits = signature.bits[i];
            static_assert(sizeof(bits) == 8);
            // no h
            uint32_t bottomHalf = (uint32_t)(bits & 0xFFFFFFFF);
            uint32_t topHalf = (uint32_t)(bits >> 32);
            while (bottomHalf != 0) {
                auto id = (ComponentID)highestBit(bottomHalf);
                code |= pools[id].add(entity.id);
                // erase bit from signature and move rest of the bits up
                signature.bits
            }
        }
        */
        return code;
    }

    int RemoveComponents(Entity entity, ComponentID* componentIDs, Uint32 numComponentIDs) {
        if (!EntityExists(entity)) {
            LogError("Attempted to remove components from a non-living entity! Entity: %s", entity.DebugStr());
            return -1;
        }

        ComponentFlags& entityFlags = entityDataList[entity.id].flags;        

        int code = 0;
        for (Uint32 i = 0; i < numComponentIDs; i++) {
            ComponentID id = componentIDs[i];
            if (entityFlags[id]) {
                code |= pools[id].remove(entity.id);
                entityFlags &= ~(1 << id);
            }
        }
        return code;
    }

    template<class... Components>
    int RemoveComponents(Entity entity) {
        if (!EntityExists(entity)) {
            LogError("EntityManager::RemoveComponents : Attempted to remove components from a non-living entity! Entity: %s", entity.DebugStr());
            return -1;
        }

        constexpr auto componentIDs = getComponentIDs<Components...>();
        ComponentFlags& entityFlags = entityDataList[entity.id].flags;

        int code = 0;
        for (Uint32 i = 0; i < componentIDs.size(); i++) {
            ComponentID id = componentIDs[i];
            if (entityFlags[id]) {
                code |= pools[id].remove(entity.id);
            }
        }

        // remove all components from the entity's flags that are in the passed signature
        entityFlags &= ~componentSignature<Components...>();
        return code;
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
};

}

#endif