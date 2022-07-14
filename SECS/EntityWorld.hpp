#ifndef ENTITY_WORLD_INCLUDED
#define ENTITY_WORLD_INCLUDED

#include "ECS.hpp"
#include "../EntityComponents/Components.hpp"

template<class... Components>
class ComponentManager;

struct GameState;

class EntityWorld;

namespace GameSave {
    int writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state);
    int readEntityDataFromFile(const char* filepath, EntityWorld* ecs);
}

template<class... Components>
class ComponentManager;

class EntityWorld {
protected:
    ECS::EntityManager* em = NULL;
    using EventCallback = std::function<void(EntityWorld*, Entity)>;
    EventCallback callbacksOnAdd[NUM_COMPONENTS];
    EventCallback callbacksBeforeRemove[NUM_COMPONENTS];

    bool deferringEvents = false;
    struct DeferredEvent {
        Entity entity;
        EventCallback callback;
    };
    std::vector<DeferredEvent> deferredEvents;
public:

    friend int GameSave::writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state);
    friend int GameSave::readEntityDataFromFile(const char* filepath, EntityWorld* ecs);

    EntityWorld() {
        em = new ECS::EntityManager();
    }

    /* Initialize the entity world.
     * It is NOT safe to use an entity world without first calling this method on it.
     * Think of this function as a constructor.
     */
    template<class... Components>
    void init() {
        em->init<Components...>();
        for (Uint32 i = 0; i < em->numComponents(); i++) {
            em->getPool(i)->setName(componentNames[i]);
        }
    }

    /* Destroy the entity world.
     * Frees memory and destroys members without doing any other work (such as calling event callbacks).
     * It is NOT safe to use an entity world after calling this method on it.
     * Essentially a destructor.
     */
    void destroy() {
        em->destroy();
        delete em;
    }

    /* Create a new entity with just an entity type component using the type name given.
     * Returned entity should never be null and should always exist.
     * @return A newly created entity.
     */
    Entity New(const char* type) {
        Entity entity = em->New();
        Add(entity, EC::EntityTypeEC(type));
        return entity;
    }

    /* Create a new entity with the components passed added, as well as an entity type component using the type name given.
     * The components will be added, but they will not be initialized in any way. If read before being written to, you'll get either old component data or garbage.
     * Returned entity should never be null and should always exist.
     * @return A newly created entity.
     */
    template<class... Components>
    EntityType<Components...> NewType(const char* type) {
        Entity entity = New(type);
        Add<Components...>(entity);
        return entity;
    }

    /* Destroy an entity, effectively removing all of its components (while triggering relevant events for those components),
     * rendering it unusable. Attempting to destroy an entity that does not exist will do nothing other than trigger an error.
     * @return 0 if the destruction was successful, -1 on failure.
     */
    int Destroy(Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("EntityWorld::Destroy : Entity passed does not exist! Entity: %s", entity.DebugStr().c_str());
            return -1;
        }

        ComponentFlags entityComponents = em->getEntityComponents(entity.id);
        for (ECS::ComponentID id = 0; id < em->numComponents(); id++) {
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
        return em->Destroy(entity);
    }

    /* Check whether an entity exists, AKA whether the entity was properly created using New and not yet Destroyed.
     * @return True if the entity exists, false if the entity is null or was destroyed.
     */
    inline bool EntityExists(Entity entity) const {
        return em->EntityExists(entity);
    }

    /* Get the entity component signature (AKA component flags) for an entity ID. 
     * Be warned this may be incorrect if the entity ID passed is for an outdated entity,
     * that is, an entity that has been destroyed and then used again with a newer version.
     * Do not use this method without knowing the entity passed has not been destroyed.
     * @return The component flags corresponding to the entity ID.
     */
    inline ECS::ComponentFlags EntitySignature(EntityID entityID) const {
        return em->getEntityComponents(entityID);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    template<class... ECs>
    inline bool EntityHas(Entity entity) const {
        return EntityExists(entity) && em->EntityHas<ECs...>(entity);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    inline bool EntityHas(Entity entity, ComponentFlags components) const {
        return (EntityExists(entity) && em->getEntityComponents(entity.id).hasAll(components));
    }

    /* Get the latest entity version in use for the given ID.
     * In the case that the entity with the id given does not exist, the version returned will be the next version to be used for that entity.
     */
    inline EntityVersion GetEntityVersion(EntityID id) const {
        return em->entityDataList[id].version;
    }

    /* Get the number of entities currently in existence.
     * This is equivalent to the size of the main entity list.
     */
    inline Uint32 EntityCount() const {
        return em->numLiveEntities();
    }

    /* Get the list containing all entities in the world, length equal to the number of entities in existence (can be found using EntityCount()).
     * Be careful using this in a concurrent context or while also destroying or creating entities, as the size of contents will not remain constant.
     * To safely iterate this list while destroying iterated entities, iterate in reverse order or use IterateEntities (it iterates in reverse order).
     * Creating entities while iterating this list is also safe only while iterating in reverse order, but keep in mind that that will not iterate the newly created entities.
     */
    const Entity* GetEntityList() const {
        return (Entity*)em->entities;
    }

    template<class T>
    T* Get(Entity entity) const {
        return em->Get<T>(entity);
    }

    template<class T>
    void Set(Entity entity, const T& value) {
        if (sizeof(T) == 0) return;
        // perhaps add a NULL check here and log an error instead of dereferencing immediately?
        // could hurt performance depending on where it's used
        *em->Get<T>(entity) = value;
    }

    template<class T>
    int Add(Entity entity, const T& startValue) {
        int ret = em->Add<T>(entity, startValue);
        if (!ret) {
            auto& onAddT = callbacksOnAdd[getID<T>()];
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

    template<class... Components>
    int Add(Entity entity) {
        constexpr auto ids = ECS::getComponentIDs<Components...>();
        int ret = em->Add<Components...>(entity);
        if (ret) {
            Log.Info("Entity type: %s", em->Get<EC::EntityTypeEC>(entity)->name);
        } else {
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
        auto& beforeRemoveT = callbacksBeforeRemove[getID<T>()];
        if (beforeRemoveT) {
            if (deferringEvents) {
                deferredEvents.push_back({entity, beforeRemoveT});
            } else {
                beforeRemoveT(this, entity);
            }
        }
        int ret = em->RemoveComponents<T>(entity);
        
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
        callbacksOnAdd[getID<T>()] = callback;
    }

    template<class T>
    void SetBeforeRemove(EventCallback callback) {
        callbacksBeforeRemove[getID<T>()] = callback;
    }

    /* Iterate all entities in the world.
     * It is safe to destroy entities while iterating.
     * Creating entities while iterating this list is also safe, but keep in mind that entities created during iteration will be skipped.
     */
    inline void IterateEntities(std::function<void(Entity)> callback) const {
        for (int e = em->numLiveEntities(); e >= 0; e--) {
            callback(em->entities[e]);
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
       EntityQueryT::ForEach(em, callback);
    }

    /* Get a pointer to the component pool for a given component type */
    template<class T>
    inline const ECS::ComponentPool* GetPool() const {
        return em->getPool<T>();
    }

    template<class... Components>
    operator ComponentManager<Components...>() const {
        return ComponentManager<Components...>(this);
    }
};

/*
template<class... Components>
struct EntityCreator : public EntityType<Components...> {
    using EntityType<Components...>::EntityType;

protected:

    EntityCreator(EntityWorld* ecs, const char* type) {
        *this = ecs->New(type);
        ecs->Add<Components...>(*this);
    }

    template<class T>
    int Add(EntityWorld* ecs, const T& startValue) const {
        return ecs->Add(*this, startValue);
    }

    template<class T>
    void Set(const EntityWorld* ecs, const T& startValue) const {
        return ecs->Set(*this, startValue);
    }

    template<class T>
    T* Get(const EntityWorld* ecs) const {
        return ecs->Get(*this);
    }
};
*/

template<class... Components>
class ComponentManager {
private:
    const EntityWorld* ecs;
public:
    ComponentManager(const EntityWorld* ecs) : ecs(ecs) {}

    inline void SetWorld(const EntityWorld* world) {
        if (!world)
            Log.Error("ComponentManager::SetWorld : World passed is null!");
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
        constexpr ECS::ComponentID id = getID<T>();
        static_assert(mutSignature[id] || componentIsConst<T>(), "need mutable signature to get mutable component");
        return ecs->Get<T>(entity);
    }

    template<class T>
    T* GetMut(Entity entity) const {
        static_assert(ECS::componentInComponents<T, Components...>(), "component manager must be manager of component");
        return ecs->Get<T>(entity);
    }

    inline bool EntityExists(Entity entity) const {
        return ecs->EntityExists(entity);
    }

    template<class... Cs>
    inline bool EntityHas(Entity entity) const {
        return ecs->EntityHas<Cs...>(entity);
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
        for (unsigned int i = 0; i < NUM_COMPONENTS; i++) {
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