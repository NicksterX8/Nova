#ifndef ENTITY_WORLD_INCLUDED
#define ENTITY_WORLD_INCLUDED

#include <vector>
#include <string>
#include <sstream>

#include "global.hpp"
#include "ECS/EntityManager.hpp"
#include "utils/common-macros.hpp"
#include "components/components.hpp"
#include "entities/prototypes/def.hpp"
#include "ECS/system.hpp"

struct ChunkMap;

namespace World {

using ECS::EntityCommandBuffer;

struct EntityWorld;

struct EntityMaker {
    EntityWorld* ecs = nullptr;
    
    Entity entity = NullEntity;
    ECS::Signature components = {0};
    ECS::PrototypeID prototype = -1;

    Uint16 bufUsed = 0;
    MutableArrayRef<char> buf;

    static constexpr size_t MaxComponentMemory = std::numeric_limits<Uint16>::max();
    struct ComponentValue {
        ECS::ComponentID id;
        Uint16 valueBufPos;
    };
    SmallVector<ComponentValue> componentValues;

    EntityMaker() {}

    EntityMaker(EntityWorld* ecs);
    
    EntityMaker(const EntityMaker& other) = delete;

    // if an entity was in progress or already made, that data will be replaced with a fresh start
    void start(ECS::PrototypeID prototype) {
        clear();
        this->prototype = prototype;
    }

    template<class C>
    void Add(const C& value) {
        components.setComponent<C>();
        if (bufUsed + sizeof(C) > buf.size()) {
            assert(0 && "Ran out of buffer space");
        }
        Uint16 offset = (Uint16)((uintptr_t)buf.data() % alignof(C));
        memcpy(&buf[bufUsed + offset], &value, sizeof(C));
        componentValues.push_back(ComponentValue{C::ID, (Uint16)(bufUsed + offset)});
        bufUsed += sizeof(C) + offset;
    }

    void setPos(Vec2 pos) {
        Add<EC::Position>(pos);
    }

    Entity make();

    void make(Entity* pEntity) {
        *pEntity = make();
    }

    Entity operator()() {
        return make();
    }

    void clear();
};

struct EntityWorld {
    ECS::EntityManager em;
protected:
    
    using EventCallback = std::function<void(EntityWorld*, ECS::Entity)>;
    
    // does nothing currently
    EventCallback callbacksOnAdd[EC::ComponentIDs::Count];
    EventCallback callbacksBeforeRemove[EC::ComponentIDs::Count];

    bool deferringEvents;
    struct DeferredEvent {
        ECS::Entity entity;
        EventCallback callback;
    };
    SmallVector<DeferredEvent, 3> deferredEvents;

    char* entityMakerBuffer;
    EntityMaker entityMaker;

    ChunkMap* chunkmap;

public:
    EntityWorld() {}

    void init(ChunkMap* chunkmap, EntityWorld* pointerToThis);

    EntityWorld(const EntityWorld& copy) = delete;

    /* Destroy the entity world.
     * Frees memory and destroys members without doing any other work (such as calling event callbacks).
     * It is NOT safe to use an entity world after calling this method on it.
     * Essentially a destructor.
     */
    void destroy() {
        em.destroy();
    }

    void setComponentDestructor(ECS::ComponentID component, const ECS::ComponentDestructor& destructor) {
        em.setComponentDestructor(component, destructor);
    }

    void useCommandBuffer(EntityCommandBuffer* buffer) {
        em.useCommandBuffer(buffer);
    }

    void flushCurrentCommandBuffer() {
        em.flushCurrentCommandBuffer();
    }

    void executeCommandBuffer(EntityCommandBuffer* buffer) {
        em.executeCommandBuffer(buffer);
    }

    ssize_t getComponentSize(ECS::ComponentID id) const {
        return em.getComponentInfo(id).size;
    }

    /* Create a new entity with just an entity type component using the type name given.
     * @return A newly created entity.
     */
    Entity New(ECS::PrototypeID prototype) {
        Entity entity = em.newEntity(prototype);
        
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

        em.deleteEntity(entity);
    }

    /* Check whether an entity exists, AKA whether the entity was properly created using New and not yet Destroyed.
     * @return True if the entity exists, false if the entity is null or was destroyed.
     */
    bool EntityExists(Entity entity) const {
        return em.entityExists(entity);
    }

    /* Get the entity component signature (AKA component flags) for an entity ID. 
     * Be warned this may be incorrect if the entity ID passed is for an outdated entity,
     * that is, an entity that has been destroyed and then used again with a newer version.
     * Do not use this method without knowing the entity passed has not been destroyed.
     * @return The component flags corresponding to the entity ID.
     */
    ECS::Signature EntitySignature(Entity entity) const {
        if (LIKELY(em.entityExists(entity)))
            return em.getEntitySignature(entity);
        return ECS::Signature(0);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    template<class... ECs>
    bool EntityHas(Entity entity) const {
        return EntityExists(entity) && em.entityHas<ECs...>(entity);
    }

    /* Check if an entity exists and has all of the given components.
     * If you already know the entity exists, you can use EntitySignature to check yourself if speed is critical.
     * @return True when the entity exists and has the components, otherwise false.
     */
    bool EntityHas(Entity entity, ECS::Signature components) const {
        return (EntityExists(entity) && em.entityHas(entity, components));
    }

    /* Get the latest entity version in use for the given ID.
     * In the case that the entity with the id given does not exist,
     * ECS::NULL_ENTITY_VERSION will be returned
     */
    ECS::EntityVersion GetEntityVersion(ECS::EntityID id) const;

    /* Get the number of entities currently in existence.
     * This is equivalent to the size of the main entity list.
     */
    inline Uint32 EntityCount() const {
        return em.components.entityData.getSize();
    }
    
    /* Get a component from the entity of the type T.
     * Completely side-effect free. Will log an error and return null if the entity does not exist or if the entity does not own a component of the type.
     * Passing a type that has not been initialized with init() will result in an error message and getting null.
     * In order to not be wasteful, types with a size of 0 (AKA flag components) will result in a return value of null.
     * @return A pointer to a component of the type or null on error.
     */
    template<class T>
    T* Get(Entity entity) const {
        return em.getComponent<T>(entity);
    }

    /* Get a component from the entity of the type T.
     * Completely side-effect free. Will log an error and return null if the entity does not exist or if the entity does not own a component of the type.
     * Passing a type that has not been initialized with init() will result in an error message and getting null.
     * In order to not be wasteful, types with a size of 0 (AKA flag components) will result in a return value of null.
     * @return A pointer to a component of the type or null on error.
     */
    void* Get(Entity entity, ECS::ComponentID component) const {
        return em.getComponent(entity, component);
    }

    template<class T>
    T* Set(Entity entity, const T& value) {
        static_assert(!std::is_const<T>(), "Component must not be const to set values!");
        if (sizeof(T) == 0) return NULL;

        T* component = em.getComponent<T>(entity);
        // perhaps add a NULL check here and log an error instead of dereferencing immediately?
        // could hurt performance depending on where it's used
        // decided to add check as otherwise this method is useless, so only use it if a null check is intended.
        if (component)
            *component = value;
        return component;
    }

    void Set(Entity entity, ECS::ComponentID componentID, void* value);

    /* Add a component of the type to the entity, immediately initializing the value to param startValue.
     * Triggers the relevant 'onAdd' event directly after adding and setting start value if not currently deferring events,
     * otherwise it will be added to the back of the deferred events queue to be executed when finished deferring.
     * @return 0 on success, any other value otherwise. A relevant error message should be logged.
     */
    template<class T>
    bool Add(Entity entity, const T& startValue) {
        if (em.addComponent<T>(entity, startValue)) {
            
            // auto& onAddT = callbacksOnAdd[ECS::getID<T>()];
            // if (onAddT) {
            //     if (deferringEvents) {
            //         deferredEvents.push_back({entity, onAddT});
            //     } else {
            //         onAddT(this, entity);
            //     }
            // }
            return true;
            
        }
        return false;
    }

    // TODO: should this exist?
    // template<class T>
    // bool Add(EntityMaker& entity, const T& startValue) {
    //     entity.Add<T>(startValue);
    //     return false;
    // }

    /* Add the component corresponding to the id to the given entity
     * Triggers the relevant 'onAdd' event directly after adding and setting start value if not currently deferring events,
     * otherwise it will be added to the back of the deferred events queue to be executed when finished deferring.
     * @return 0 on success, any other value otherwise. A relevant error message should be logged.
     */
    bool Add(Entity entity, ECS::ComponentID id, void* value = nullptr) {
        bool ret = em.addComponent(entity, id, value);
        // if (ret) {
        //     auto& onAddT = callbacksOnAdd[id];
        //     if (onAddT) {
        //         if (deferringEvents) {
        //             deferredEvents.push_back({entity, onAddT});
        //         } else {
        //             onAddT(this, entity);
        //         }
        //     }
        // }
        return ret;
    }

    bool AddSignature(Entity entity, ECS::Signature signature) {
        if (em.addSignature(entity, signature)) {
            
            return true;
        }
        return false;
    }

    /* Add the template argument components to the entity.
     * Triggers the relevant 'onAdd' events directly after adding all of the components if not currently deferring events.
     * Keep in mind the components will be left unitialized, meaning you may get either garbage or old component data if read to before being written to.
     * If deferring events, the events will be added to the back of the deferred events queue to be executed when finished deferring in the order of the template arugments passed.
     * The events will not be triggered if adding any of the components failed.
     * @return 0 on success, -1 if adding any one of the components failed. A relevant error message should be logged.
     */
    template<class... Components>
    bool Add(Entity entity, Components... components) {
        constexpr ECS::Signature signature = ECS::getSignature<Components...>();
        bool ret = em.addSignature(entity, signature);
        if (ret) {
            void* ptrs[] = {(void*)Set<Components>(entity, components) ...};
        }

        return ret;
    }

    template<class T>
    void Remove(Entity entity) {
        
        auto& beforeRemoveT = callbacksBeforeRemove[ECS::getID<T>()];
        if (beforeRemoveT) {
            if (deferringEvents) {
                deferredEvents.push_back({entity, beforeRemoveT});
            } else {
                beforeRemoveT(this, entity);
            }
        }
        
        em.removeComponent<T>(entity);
    }

    const Prototype* getPrototype(ECS::PrototypeID prototype) {
        return (Prototype*)em.getPrototype(prototype);
    }

    const Prototype* getPrototype(const char* prototypeName) {
        return (Prototype*)em.getPrototype(prototypeName);
    }

    const Prototype* getPrototype(Entity entity) {
        return (Prototype*)em.getPrototype(entity);
    }

    EntityMaker& startMakingEntity(ECS::PrototypeID prototype) {
        entityMaker.start(prototype);
        return entityMaker;
    }

    EntityMaker* getEntityMaker() {
        return &entityMaker;
    }

    MutableArrayRef<char> getEntityMakerBuffer() const {
        return MutableArrayRef(entityMakerBuffer, 1024);
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

    void ForEachAll(std::function<bool(Entity entity)> callback) const {
        em.forAllEntities(callback);
    } 

    /* Iterate entities filtered using an EntityQuery.
     * It is safe to destroy entities while iterating.
     * Creating entities while iterating is also safe, but keep in mind that entities created during iteration will be skipped.
     * Iteration is significantly more efficient when atleast one very uncommon component is required.
     * Entity passed to the callback uses a wildcard version instead of a real version for performance reasons.
     * As such, version comparisons may not work as expected.
     */
    inline void ForEach(std::function<bool(ECS::Signature)> query, std::function<void(Entity entity)> callback) const {
        em.forEachEntity(query, callback);
    }

    /* Iterate entities filtered using an EntityQuery.
     * It is safe to destroy entities while iterating.
     * Creating entities while iterating is also safe, but keep in mind that entities created during iteration will be skipped.
     * Iteration is significantly more efficient when atleast one very uncommon component is required.
     * Entity passed to the callback uses a wildcard version instead of a real version for performance reasons.
     * As such, version comparisons may not work as expected.
     * Return false in the callback to continue iterating
     * Return true to break and stop iterating
     */
    inline void ForEach_EarlyReturn(std::function<bool(ECS::Signature)> query, std::function<bool(Entity entity)> callback) const {
        em.forEachEntity_EarlyReturn(query, callback);
    }

    ECS::ComponentID GetComponentIdFromName(const char* name) const {
        return em.getComponentIdFromName(name);
    }

    template<typename T>
    void setValue(void* valuePtr, T value) const {
        memcpy(valuePtr, &value, sizeof(T));
    }

    bool GetComponentValueFromStr(ECS::ComponentID component, std::string str, void* v) const {
        if (component == ECS::NullComponentID) return false;
        auto componentSize = getComponentSize(component);
        bool empty = str.empty() || str == "{}";
        

        using namespace World;
        #define CASE(component_name) case ECS::getID<EC::component_name>()
        #define VALUE(component_name, ...) setValue<EC::component_name>(v, __VA_ARGS__); return true
        std::stringstream ss(str);
        // TODO: for objects automatically parse out properties and stuff. make it an array
        // OR / AND REGEX? dont use std library one casue it sucks balllssss

        switch (component) {

        CASE(Position): {
            if (empty) {
                VALUE(Position, {{0.0f, 0.0f}});
            }

            float x,y;

            ss.ignore(1, '{');
            ss >> x;
            ss.ignore(1, ',');
            ss >> y;

            VALUE(Position, {x, y});
        }
        /*
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
        */
        }

        return false;
    }
};

struct EntityBuilder {
    EntityWorld* ecs;

    EntityBuilder(EntityWorld* ecs) : ecs(ecs) {}

    EntityMaker& New(ECS::PrototypeID prototype) {
        return ecs->startMakingEntity(prototype);
    }
};

}

using World::EntityWorld;

#endif