#ifndef ENTITY_MANAGER_INCLUDED
#define ENTITY_MANAGER_INCLUDED

#include <stdlib.h>
#include <stdint.h>
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



typedef uint32_t Uint32;

namespace ECS {

using EntityCallback = std::function<void(Entity)>;

class EntityManager {
public:
    Entity entities[MAX_ENTITIES];
    struct EntityData {
        EntityVersion version; // The current entity version
        ComponentFlags flags; // The entity's component signature
        Uint32 index; // Where in the entities array the entity is stored
    } entityDataList[MAX_ENTITIES]; // The list of data corresponding to an entity ID, indexed by the ID

    Uint32 liveEntities = 0; // The number of alive (or existent) entities
    std::vector<EntityID> freeEntities = std::vector<EntityID>(MAX_ENTITIES-1); // A stack of free entity ids

    // TODO: allocate pools in an array, find a solution to the optional pool problem.
    ComponentPool** pools = NULL; // a list of component pool pointers
    Uint32 nComponents = 0; // The number of unique component types, the size of the pools array
    ComponentID highestComponentID = 0; // The highest id of all the components used in the entity manager. Should be equal to nComponents most of the time

public:
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
        if (id > nComponents) {
            Log.Error("EntityManager::getPool id passed is too high, could not get component pool!");
            return NULL;
        }
        return pools[id];
    }

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
        if (id > highestComponentID)
            highestComponentID = id;
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
        pools = new ComponentPool*[sizeof...(Components)];
        pools = (ComponentPool**)calloc(sizeof...(Components), sizeof(ComponentPool*));
        newComponents<Components...>();

        static_assert(NULL_ENTITY_VERSION == 0, "starting entity version should be 1");

        // set all entities to null at start just in case, even though these should be overwritten before they're read.
        for (EntityID i = 0; i < MAX_ENTITIES; i++) {
            entities[i] = NullEntity;
        }
        // the version must be set, as it is used when creating new entities.
        // the flags will be set to 0 again so it's a redundant safety measure.
        // the index is required to show that the entities are not in existence.
        for (EntityID i = 0; i < MAX_ENTITIES; i++) {
            entityDataList[i] = {
                .version = 1,
                .flags = ComponentFlags(0),
                .index = NULL_ENTITY_ID
            };
        }
        // all entities are free by default.
        // stop one shorter than the others to avoid making NULL_ENTITY_ID a free entity.
        for (EntityID i = 0; i < NULL_ENTITY_ID; i++) {
            // set free entities in reverse order so that when new entities are popped off the back the ids will go from lowest to highest,
            // starting at 0, ending at NULL_ENTITY_ID-1
            freeEntities[(NULL_ENTITY_ID-1)-i] = i;
        }
    }

    // destroy the entity manager and deallocate everything
    void destroy();

    inline Uint32 numComponents() const {
        return nComponents;
    }

    Uint32 numLiveEntities() const;

    ComponentFlags getEntityComponents(EntityID entityID) const;

    template<class... Cs>
    inline bool EntityHas(Entity entity) const {
        constexpr ComponentFlags signature = componentSignature<Cs...>();
        return getEntityComponents(entity.id).hasAll(signature);
    }

    inline bool EntityExists(Entity entity) const {
        //static_assert(NULL_ENTITY_VERSION < RESERVED_ENTITY_VERSION, "Null entity version entity should not exist");
        return (entity.id < NULL_ENTITY_ID && entity.version >= entityDataList[entity.id].version);
    }

    /* Get the name of the component type */
    template<class T>
    const char* getComponentName() const {
        return getComponentName(getID<T>());
    }

    /* Get the name of the component type corresponding to the id */
    const char* getComponentName(ComponentID id) const {
        if (id > highestComponentID) return NULL;
        return getPool(id)->name;
    }

    /* Get the size of the component pool for the type */
    template<class T>
    Uint32 componentPoolSize() const {
        return getPool<T>()->getSize();
    }

    Entity getEntityByIndex(Uint32 entityIndex) const;

    Entity New();

    int Destroy(Entity entity);

    template<class T> 
    T* Get(Entity entity) const {
        if (sizeof(T) == 0) return NULL;

        if (EntityExists(entity))
            return static_cast<T*>(getPool<T>()->get(entity.id));

        Log.Error("Attempted to get unowned component \"%s\" of an entity! Returning NULL. Entity: %s", getComponentName<T>(), entity.DebugStr().c_str());
        return NULL;
    }

    void* Get(ComponentID componentID, Entity entity) const {
        ComponentPool* pool = getPool(componentID);
        if (EntityExists(entity) && pool)
            return pool->get(entity.id);

        Log.Error("Failed to get component \"%s\" of an entity! Returning NULL. Entity: %s", getComponentName(componentID), entity.DebugStr().c_str());
        return NULL;
    }

    template<class... Components>
    int Add(Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::Add : Attempted to add components to a non-living entity! Entity: %s.", entity.DebugStr().c_str());
            return -1;
        }

        ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
        // start at one to account for dummy value
        int code = 0;
        for (Uint32 i = 1; i < sizeof(ids) / sizeof(ComponentID); i++) {
            ComponentID id = ids[i];
            code |= pools[id]->add(entity.id);
        }
        entityDataList[entity.id].flags |= componentSignature<Components...>();
        return code;
    }

    template<class T>
    int Add(Entity entity, const T& startValue) {
        if (sizeof(T) == 0) return 1;

        if (!EntityExists(entity)) {
            Log.Error("EntityManager::Add %s : Attempted to add a component to a non-living entity! Entity: %s", getComponentName<T>(), entity.DebugStr().c_str());
            return -1;
        }

        entityDataList[entity.id].flags |= componentSignature<T>();
        if (!getPool<T>()->add(entity.id))
            *static_cast<T*>(getPool<T>()->get(entity.id)) = startValue;

        return 0;
    }

    int AddSignature(Entity entity, ComponentFlags signature) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::AddSignature : Attempted to add components to a non-existent entity! Entity: %s", entity.DebugStr().c_str());
            return -1;
        }
        
        // just in case the same component is tried to be added twice
        ComponentFlags* components = &entityDataList[entity.id].flags;
        ComponentFlags newComponents = (signature ^ *components) & signature;
        *components |= signature;
        int code = 0;
        for (ComponentID id = 0; id < nComponents; id++) {
            if (signature[id])
                code |= pools[id]->add(entity.id);
        }
        return code;
    }

    int RemoveComponents(Entity entity, ComponentID* componentIDs, Uint32 numComponentIDs) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::RemoveComponents : Attempted to remove components from a non-living entity! Entity: %s", entity.DebugStr().c_str());
            return -1;
        }

        ComponentFlags& entityFlags = entityDataList[entity.id].flags;        

        int code = 0;
        for (Uint32 i = 0; i < numComponentIDs; i++) {
            ComponentID id = componentIDs[i];
            if (entityFlags[id]) {
                code |= pools[id]->remove(entity.id);
                entityFlags.set(id, 0);
            }
        }
        return code;
    }

    template<class... Components>
    int RemoveComponents(Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("EntityManager::RemoveComponents : Attempted to remove components from a non-living entity! Entity: %s", entity.DebugStr().c_str());
            return -1;
        }

        auto componentIDs = getComponentIDs<Components...>();
        ComponentFlags& entityFlags = entityDataList[entity.id].flags;

        int code = 0;
        for (Uint32 i = 0; i < componentIDs.size(); i++) {
            ComponentID id = componentIDs[i];
            if (entityFlags[id]) {
                code |= pools[id]->remove(entity.id);
            }
        }

        // remove all components from the entity's flags that are in the passed signature
        entityFlags &= ~componentSignature<Components...>();
        return code;
    }

    // garbo     

    template<class T, class... EntityComponents> 
    T* Get(_EntityType<EntityComponents...> entity) {
        static_assert(componentInComponents<T, EntityComponents...>(),
            "Entity must have correct type to get a component!");

        return Get<T>(entity);
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
};

}

#endif