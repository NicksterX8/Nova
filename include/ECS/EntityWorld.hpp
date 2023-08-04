#ifndef ENTITY_WORLD_INCLUDED
#define ENTITY_WORLD_INCLUDED

#include "utils/common-macros.hpp"
#include "SECS.hpp"
#include "components/components.hpp"
#include <vector>
#include <string>
#include <sstream>

#include "global.hpp"

template<class... Components>
struct ComponentManager;

struct GameState;

struct EntityWorld;

namespace GameSave {
    int writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state);
    int readEntityDataFromFile(const char* filepath, EntityWorld* ecs);
}

template<class... Components>
struct ComponentManager;

struct EntityWorld {
protected:
    ECS::EntityManager em;
    using EventCallback = std::function<void(EntityWorld*, Entity)>;
    EventCallback callbacksOnAdd[ECS_NUM_COMPONENTS];
    EventCallback callbacksBeforeRemove[ECS_NUM_COMPONENTS];

    bool deferringEvents = false;
    struct DeferredEvent {
        Entity entity;
        EventCallback callback;
    };
    llvm::SmallVector<DeferredEvent, 3> deferredEvents;
public:

    friend int GameSave::writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state);
    friend int GameSave::readEntityDataFromFile(const char* filepath, EntityWorld* ecs);

    /* Initialize the entity world.
     * It is NOT safe to use an entity world without first calling this method on it.
     * Think of this function as a constructor.
     */
    template<class... Components>
    static EntityWorld Init() {
        EntityWorld self;
        self.em = ECS::EntityManager::Init<Components...>();
        for (Uint32 i = 0; i < self.em.getComponentCount(); i++) {
            ECS::ComponentPool* pool = self.em.getPool(i);
            if (pool) {
                pool->name = componentNames[i];

            }
        }
        return self;
    }

    /* Destroy the entity world.
     * Frees memory and destroys members without doing any other work (such as calling event callbacks).
     * It is NOT safe to use an entity world after calling this method on it.
     * Essentially a destructor.
     */
    void destroy() {
        em.destroy();
    }

    Sint32 getComponentSize(ECS::ComponentID id) const {
        auto pool = em.getPool(id);
        return pool ? pool->getValueSize() : 0;
    }

    /* Create a new entity with just an entity type component using the type name given.
     * Returned entity should never be null and should always exist.
     * @return A newly created entity.
     */
    Entity New(const char* type) {
        Entity entity = em.New();
        Add(entity, EC::EntityTypeEC(type));
        Add(entity, EC::Fresh());
        return entity;
    }

    /* Create a new entity with the components passed added, as well as an entity type component using the type name given.
     * The components will be added, but they will not be initialized in any way. If read before being written to, you'll get either old component data or garbage.
     * Returned entity should never be null and should always exist.
     * @return A newly created entity.
     */
    template<class... Components>
    EntityT<Components...> NewType(const char* type) {
        Entity entity = New(type);
        Add<Components...>(entity);
        return entity;
    }

    /* Destroy an entity, effectively removing all of its components (while triggering relevant events for those components),
     * rendering it unusable. Attempting to destroy an entity that does not exist will do nothing other than trigger an error.
     * @return 0 if the destruction was successful, -1 on failure.
     */
    void Destroy(Entity entity) {
        if (!EntityExists(entity)) {
            LogError("EntityWorld::Destroy : Entity passed does not exist! Entity: %s", entity.DebugStr());
            return;
        }

        ComponentFlags entityComponents = em.EntitySignature(entity.id);
        for (ECS::ComponentID id = 0; id < em.getComponentCount(); id++) {
            if (entityComponents[id]) {
                auto& beforeRemoveT = callbacksBeforeRemove[id];
                if (beforeRemoveT) {
                    if (deferringEvents) {
                        deferredEvents.push_back({entity, beforeRemoveT});
                    } else {
                        beforeRemoveT(this, entity);
                    }
                }
            }
        }
        em.Destroy(entity);
    }

    /* Check whether an entity exists, AKA whether the entity was properly created using New and not yet Destroyed.
     * @return True if the entity exists, false if the entity is null or was destroyed.
     */
    bool EntityExists(Entity entity) const {
        return em.EntityExists(entity);
    }

    /* Get the entity component signature (AKA component flags) for an entity ID. 
     * Be warned this may be incorrect if the entity ID passed is for an outdated entity,
     * that is, an entity that has been destroyed and then used again with a newer version.
     * Do not use this method without knowing the entity passed has not been destroyed.
     * @return The component flags corresponding to the entity ID.
     */
    ECS::ComponentFlags EntitySignature(EntityID entityID) const {
        return em.EntitySignature(entityID);
    }

    /* Get the entity component signature (AKA component flags) for an entity ID. 
     * Be warned this may be incorrect if the entity ID passed is for an outdated entity,
     * that is, an entity that has been destroyed and then used again with a newer version.
     * Do not use this method without knowing the entity passed has not been destroyed.
     * @return The component flags corresponding to the entity ID.
     */
    ECS::ComponentFlags EntitySignature(Entity entity) const {
        if (LIKELY(em.EntityExists(entity)))
            return em.EntitySignature(entity.id);
        return ECS::ComponentFlags(0);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    template<class... ECs>
    bool EntityHas(Entity entity) const {
        return EntityExists(entity) && em.EntityHas<ECs...>(entity);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    inline bool EntityHas(Entity entity, ComponentFlags components) const {
        return (EntityExists(entity) && em.EntitySignature(entity.id).hasAll(components));
    }

    /* Get the latest entity version in use for the given ID.
     * In the case that the entity with the id given does not exist, the version returned will be the next version to be used for that entity.
     */
    inline EntityVersion GetEntityVersion(EntityID id) const {
        return em.entityDataList[id].version;
    }

    /* Get the number of entities currently in existence.
     * This is equivalent to the size of the main entity list.
     */
    inline Uint32 EntityCount() const {
        return em.getEntityCount();
    }

    /* Get the list containing all entities in the world, length equal to the number of entities in existence (can be found using EntityCount()).
     * Be careful using this in a concurrent context or while also destroying or creating entities, as the size of contents will not remain constant.
     * To safely iterate this list while destroying iterated entities, iterate in reverse order or use IterateEntities (it iterates in reverse order).
     * Creating entities while iterating this list is also safe only while iterating in reverse order, but keep in mind that that will not iterate the newly created entities.
     */
    const Entity* GetEntityList() const {
        return (Entity*)em.entities;
    }

    /* Get a component from the entity of the type T.
     * Completely side-effect free. Will log an error and return null if the entity does not exist or if the entity does not own a component of the type.
     * Passing a type that has not been initialized with init() will result in an error message and getting null.
     * In order to not be wasteful, types with a size of 0 (AKA flag components) will result in a return value of null.
     * @return A pointer to a component of the type or null on error.
     */
    template<class T>
    T* Get(Entity entity) const {
        return em.Get<T>(entity);
    }

    /* Get a component from the entity of the type T.
     * Completely side-effect free. Will log an error and return null if the entity does not exist or if the entity does not own a component of the type.
     * Passing a type that has not been initialized with init() will result in an error message and getting null.
     * In order to not be wasteful, types with a size of 0 (AKA flag components) will result in a return value of null.
     * @return A pointer to a component of the type or null on error.
     */
    void* Get(EC_ID id, Entity entity) const {
        return em.Get(id, entity);
    }

    template<class T>
    T* Set(Entity entity, const T& value) {
        static_assert(std::is_const<T>(), "Component must be const to set values!");
        if (sizeof(T) == 0) return NULL;

        T* component = em.Get<T>(entity);
        // perhaps add a NULL check here and log an error instead of dereferencing immediately?
        // could hurt performance depending on where it's used
        // decided to add check as otherwise this method is useless, so only use it if a null check is intended.
        if (component)
            *component = value;
        return component;
    }

    /* Add a component of the type to the entity, immediately initializing the value to param startValue.
     * Triggers the relevant 'onAdd' event directly after adding and setting start value if not currently deferring events,
     * otherwise it will be added to the back of the deferred events queue to be executed when finished deferring.
     * @return 0 on success, any other value otherwise. A relevant error message should be logged.
     */
    template<class T>
    bool Add(Entity entity, const T& startValue) {
        //LogInfo("Adding %s to entity: %s", em.getComponentName<T>(), entity.DebugStr());
        if (em.Add<T>(entity)) {
            *em.Get<T>(entity) = startValue;
            auto& onAddT = callbacksOnAdd[ECS::getID<T>()];
            if (onAddT) {
                if (deferringEvents) {
                    deferredEvents.push_back({entity, onAddT});
                } else {
                    onAddT(this, entity);
                }
            }
            return true;
        }
        return false;
    }

    /* Add the component corresponding to the id to the given entity
     * Triggers the relevant 'onAdd' event directly after adding and setting start value if not currently deferring events,
     * otherwise it will be added to the back of the deferred events queue to be executed when finished deferring.
     * @return 0 on success, any other value otherwise. A relevant error message should be logged.
     */
    bool Add(ECS::ComponentID id, Entity entity) {
        //LogInfo("Adding %s to entity: %s", em.getComponentName<T>(), entity.DebugStr());
        bool ret = em.Add(id, entity);
        if (ret) {
            auto& onAddT = callbacksOnAdd[id];
            if (onAddT) {
                if (deferringEvents) {
                    deferredEvents.push_back({entity, onAddT});
                } else {
                    onAddT(this, entity);
                }
            }
        }
        return ret;
    }

    int AddSignature(Entity entity, ECS::ComponentFlags signature) {
        // TODO: idk
        UNFINISHED_CRASH();
    }

    /* Add the template argument components to the entity.
     * Triggers the relevant 'onAdd' events directly after adding all of the components if not currently deferring events.
     * Keep in mind the components will be left unitialized, meaning you may get either garbage or old component data if read to before being written to.
     * If deferring events, the events will be added to the back of the deferred events queue to be executed when finished deferring in the order of the template arugments passed.
     * The events will not be triggered if adding any of the components failed.
     * @return 0 on success, -1 if adding any one of the components failed. A relevant error message should be logged.
     */
    template<class... Components>
    int Add(Entity entity) {
        constexpr auto ids = ECS::getComponentIDs<Components...>();
        bool ret = em.Add<Components...>(entity);
        if (ret) {
            for (size_t i = 0; i < ids.size(); i++) {
                auto& onAddT = callbacksOnAdd[ids[i]];
                if (onAddT) {
                    if (deferringEvents) {
                        deferredEvents.push_back({entity, onAddT});
                    } else {
                        onAddT(this, entity);
                    }
                }
            }
        }

        return ret;
    }

    template<class T>
    int Remove(Entity entity) {
        auto& beforeRemoveT = callbacksBeforeRemove[ECS::getID<T>()];
        if (beforeRemoveT) {
            if (deferringEvents) {
                deferredEvents.push_back({entity, beforeRemoveT});
            } else {
                beforeRemoveT(this, entity);
            }
        }
        int ret = em.RemoveComponents<T>(entity);
        
        return ret;
    }

    void StartDeferringEvents() {
        deferringEvents = true;
    }

    void StopDeferringEvents() {
        for (auto& deferredEvent : deferredEvents) {
            deferredEvent.callback(this, deferredEvent.entity);
        }
        deferredEvents.clear();

        deferringEvents = false;
    }

    template<class T>
    void SetOnAdd(EventCallback callback) {
        callbacksOnAdd[ECS::getID<T>()] = callback;
    }

    template<class T>
    void SetBeforeRemove(EventCallback callback) {
        callbacksBeforeRemove[ECS::getID<T>()] = callback;
    }

    /* Iterate all entities in the world.
     * It is safe to destroy entities while iterating.
     * Creating entities while iterating this list is also safe,
     * but keep in mind that entities created during iteration will not themselves be iterated over.
     */
    template<class F>
    inline void IterateEntities(const F& callback) const {
        for (int e = em.getEntityCount()-1; e >= 0; e--) {
            callback(em.entities[e]);
        }
    }

    /* Iterate entities filtered using an EntityQuery.
     * It is safe to destroy entities while iterating.
     * Creating entities while iterating is also safe, but keep in mind that entities created during iteration will be skipped.
     * Iteration is significantly more efficient when atleast one very uncommon component is required.
     * Entity passed to the callback uses a wildcard version instead of a real version for performance reasons.
     * As such, version comparisons may not work as expected.
     */
    template<typename EntityQueryT>
    inline void ForEach(std::function<void(Entity entity)> callback) const {
       EntityQueryT::ForEach(&em, callback);
    }

    /* Get a pointer to the component pool for a given component type */
    template<class T>
    inline const ECS::ComponentPool* GetPool() const {
        return em.getPool<T>();
    }

    /* Get a pointer to the component pool for a given component type */
    inline const ECS::ComponentPool* GetPool(ECS::ComponentID id) const {
        return em.getPool(id);
    }

    ECS::ComponentID GetComponentIdFromName(const char* name) const {
        for (ComponentID id = 0; id < ECS_NUM_COMPONENTS; id++) {
            if (My::streq(&componentNames[id][4], name)) {
                return id;
            }
        }
        return ECS::NullComponentID;
    }

    inline Uint32 NumComponentPools() const {
        return em.nComponents;
    }

    template<typename T>
    void setValue(void* valuePtr, T value) const {
        memcpy(valuePtr, &value, sizeof(T));
    }

    bool GetComponentValueFromStr(ComponentID component, std::string str, void* v) const {
        if (component == NullComponentID) return false;
        auto componentSize = getComponentSize(component);
        bool empty = str.empty() || str == "{}";
        

        using namespace EC;
        #define CASE(component_name) case ECS::getID<EC::component_name>()
        #define VALUE(component_name, ...) setValue<component_name>(v, __VA_ARGS__); return true
        std::stringstream ss(str);
        // TODO: for objects automatically parse out properties and stuff. make it an array
        // OR / AND REGEX? dont use std library one casue it sucks balllssss
        switch (component) {
        CASE(Position): {
            if (empty) {
                VALUE(Position, {0.0f, 0.0f});
            }

            float x,y;

            ss.ignore(1, '{');
            ss >> x;
            ss.ignore(1, ',');
            ss >> y;

            VALUE(Position, {x, y});
        }
        CASE(Size):
            if (empty) {
                VALUE(Size, {0.0f, 0.0f});
            }

            float width,height;

            ss.ignore(1, '{');
            ss >> width;
            ss.ignore(1, ',');
            ss >> height;

            VALUE(Size, {width, height});
        }

        return false;
    }

    template<class... Components>
    operator ComponentManager<Components...>() const {
        return ComponentManager<Components...>(this);
    }

    void minmizeMemoryUsage() {
        for (Uint32 i = 0; i < em.nComponents; i++) {
            ECS::ComponentPool* pool = em.getPool(i);
            if (pool)
                pool->reallocate(pool->size());
        }
    }
};

template<class... Components>
struct ComponentManager {
private:
    const EntityWorld* ecs;
public:
    ComponentManager(const EntityWorld* ecs) : ecs(ecs) {}

    inline void SetWorld(const EntityWorld* world) {
        if (!world)
            LogError("ComponentManager::SetWorld : World passed is null!");
        ecs = world;
    }

    template<size_t V>
    void fail() const {
        static_assert(!V, "v");
    }

    template<class T>
    T* Get(Entity entity) const {
        static_assert(ECS::componentInComponents<T, Components...>(), "component manager must be manager of component");
        constexpr ComponentFlags mutSignature = ECS::componentMutSignature<Components...>();
        constexpr ECS::ComponentID id = ECS::getID<T>();
        static_assert(mutSignature[id] || std::is_const<T>(), "need mutable signature to get mutable component");
        return ecs->Get<T>(entity);
    }

    inline bool EntityExists(Entity entity) const {
        return ecs->EntityExists(entity);
    }

    template<class... Cs>
    inline bool EntityHas(Entity entity) const {
        return ecs->EntityHas<Cs...>(entity);
    }

    inline ComponentFlags EntitySignature(Entity entity) const {
        return ecs->EntitySignature(entity.id);
    }

    template<class Q>
    inline void ForEach(const ECS::EntityCallback& callback) const {
        ecs->ForEach<Q>(callback);
    }

private:
    // check if the component manager can be casted from the components Cs. The components must have all the components this manager has,
    // and not have any const components when a mutable component is necessary.
    template<class... Cs>
    static constexpr bool castCheck() {
        constexpr ECS::ComponentFlags otherFlags = ECS::componentSignature<Cs...>();
        constexpr ECS::ComponentFlags flags = ECS::componentSignature<Components...>();

        constexpr ECS::ComponentFlags otherMutFlags = ECS::componentMutSignature<Cs...>();
        constexpr ECS::ComponentFlags mutFlags = ECS::componentMutSignature<Components...>();
        if (!otherFlags.hasAll(flags)) return false;
        if (!(mutFlags.count() <= otherMutFlags.count())) return false;
        for (unsigned int i = 0; i < flags.size(); i++) {
            if (flags[i]) {
                if (!otherFlags[i]) return false;
                if (mutFlags[i]) {
                    if (!otherMutFlags[i]) return false;
                }
            }
        }

        return true;        
    }

public:

    template<class T>
    inline ECS::ComponentPool* GetPool() {
        static_assert(ECS::componentInComponents<T, Components...>(), "component manager must be manager of component");
        return ecs->GetPool<T>();
    }

    template<class... Cs>
    constexpr ComponentManager(const ComponentManager<Cs...>& other) {
        constexpr bool castCheckResult = castCheck<Cs...>();
        static_assert(castCheckResult, "cast failed!");
        // THIS IS BAD, LOOK FOR ALTERNATIVES
        ecs = ((const ComponentManager<Components...>*)&other)->ecs;
    }
};

#endif