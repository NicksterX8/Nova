#ifndef ECS_INCLUDED
#define ECS_INCLUDED

#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stddef.h>
#include <bitset>
#include <functional>
#include <vector>

#include "../constants.hpp"
#include "../NC/cpp-vectors.hpp"
#include "../Textures.hpp"
#include "../Log.hpp"

#include "Entity.hpp"
#include "Component.hpp"
#include "SystemsConstants.hpp"
#include "EntityManager.hpp"
#include "EntitySystemInterface.hpp"
#include "ComponentPool.hpp"
#include "../ComponentMetadata/component.hpp"

#include "../Chunks.hpp"

namespace Tests {
    class ECSTest;
}

class ECS {
public:
    EntityManager manager;
    EntitySystemInterface* systems[NUM_SYSTEMS] = {NULL};
    std::vector<_Entity> entitiesToBeRemoved;
    std::vector< std::function<int(void)> > componentsToBeRemoved;

    ChunkMap* chunkmap;

    ECS() {}

    template<class... Components>
    void init() {
        manager.init<Components...>();
    }

    template<class... Systems>
    void NewSystems() {
        int dummy[] = {0, (NewSystemT<Systems>(), 0) ...};
        (void)dummy;
    }

    template<class T>
    void NewSystemT() {
        if (!systems[getSystemID<T>()])
            systems[getSystemID<T>()] = new T();
    }

    void destroy();

    template<class T>
    void NewSystem(T* system) {
        systems[getSystemID<T>()] = system;
    }
    
    inline Uint32 numLiveEntities() {
        return manager.numLiveEntities();
    }

    inline bool EntityLives(_Entity entity) {
        return manager.EntityLives(entity);
    }

    inline _Entity New() {
        return manager.New();
    }

    template<class T> 
    inline T* Get(_Entity entity) {
        return manager.Get<T>(entity);
    }

    template<class T>
    inline void iterateComponents(std::function<void(T*)> callback) {
        manager.iterateComponents<T>(callback);
    }

private:
    void entitySignatureChanged(_Entity entity);
public:

    template<class... Components>
    inline int Add(_Entity entity) {
        int result = manager.Add<Components...>(entity);
        entitySignatureChanged(entity);
        return 0;
    }

    template<class T>
    inline int Add(_Entity entity, T startValue) {
        int result = manager.Add<T>(entity, startValue);
        entitySignatureChanged(entity);
        return result;
    }

    template<class... Components>
    inline int AddSignature(_Entity entity, ComponentFlags signature) {
        int result = manager.AddSignature<Components...>(entity, signature);
        entitySignatureChanged(entity);
        return result;
    }

    template<class First, class... Rest>
    int Remove(_Entity entity) {
        int result = manager.Remove<First, Rest...>(entity);
        for (Uint32 id = 0; id < NUM_SYSTEMS; id++) {
            //if (systems[id]->Query(manager.entityComponents(entity.id)))
            systems[id]->RemoveEntity(entity);
        }
        return result;
    }

    template<class... Components>
    inline int RemoveComponents(_Entity entity) {
        int result = manager.RemoveComponents<Components...>(entity);
        entitySignatureChanged(entity);
        return result;
    }

    /*
    inline int RemoveComponents(Entity entity, ComponentFlags signature) {
        int result = manager.RemoveComponents(entity, signature);
        entitySignatureChanged(entity);
        return result;
    }
    */

    /*
    * Scheduling removes isn't necessary at this time, but it might be needed in the future
    * OK it is useful for actually plowing through components instead of stopping and removing entities,
    * you can just schedule them to go all at once
    */
    
    void ScheduleRemove(_Entity entity);

    template<class First, class... Components>
    int DoScheduledRemoves() {
        if (entitiesToBeRemoved.size() > 0) {
            //Log("size of scheduled removes: %d", entitiesToBeRemoved.size());
        }
        int code = 0;
        for (auto lambdaRemove : componentsToBeRemoved) {
            code |= lambdaRemove();
        }

        for (_Entity entity : entitiesToBeRemoved) {
            code |= Remove<First, Components...>(entity);
        }

        entitiesToBeRemoved.clear();
        componentsToBeRemoved.clear();
        return code;
    }

    template<class... Components>
    void ScheduleRemoveComponents(_Entity entity) {
        auto lambda = [this, entity](){
            return RemoveComponents<Components...>(entity);
        };

        componentsToBeRemoved.push_back(lambda);
    }


    template<class T>
    inline Uint32 componentPoolSize() {
        return manager.componentPoolSize<T>();
    }

    inline _Entity getEntityByIndex(Uint32 entityIndex) {
        return manager.getEntityByIndex(entityIndex);
    }

    inline ComponentFlags entityComponents(EntityID entityId) const {
        return manager.entityComponents(entityId);
    }

    // system stuff

    template<class T>
    T* System() {
        T* system = static_cast<T*>(systems[getSystemID<T>()]);
        if (!system) {
            LogError("ECS::System : Attempted to get a NULL system (id: %u).", getSystemID<T>());
            return NULL;
        }
        return system;
    }

    // other stuff

    EntityManager* em();

    bool testInitialization() {
        for (Uint32 i = 0; i < NUM_SYSTEMS; i++) {
            if (!systems[i]) {
                LogError("Test of initialization failed! system number %u was NULL.", i);
                return false;
            }
        }

        return true;
    }

    // garbo

    // template<class... Components>
    // using ComponentCallback = std::function<void(Components...)>();

    template<class T, class... EntityComponents>
    _EntityType<T, EntityComponents...>& AddT(_EntityType<EntityComponents...> entity) {
        static_assert(!componentInComponents<T, EntityComponents...>(), "can't add component twice");
        Add<T>(entity);
        return entity.template cast<T, EntityComponents...>();
    }

    template<class T, class... EntityComponents>
    _EntityType<T, EntityComponents...>& AddT(_EntityType<EntityComponents...> entity, T startValue) {
        static_assert(!componentInComponents<T, EntityComponents...>(), "can't add component twice");
        Add<T>(entity, startValue);
        return entity.template cast<T, EntityComponents...>();
    } 
};

template<class... Components>
bool checkEntityType(_Entity entity, ECS* ecs) {
    constexpr ComponentFlags typeFlags = componentSignature<Components...>();
    if ((typeFlags & ecs->entityComponents(entity.id)) != typeFlags) {
        // Fail, missing some components
        return false;
    }
    return true;
}

template<class... Components>
bool checkEntityType(_EntityType<Components...> entity, ECS* ecs) {
    constexpr ComponentFlags typeFlags = componentSignature<Components...>();
    if ((typeFlags & ecs->entityComponents(entity.id)) != typeFlags) {
        // Fail, missing some components
        return false;
    }
    return true;
}

template<class... Components>
inline bool assertEntityType(_Entity entity, ECS* ecs) {
    assert(checkEntityType<Components...>(entity, ecs) && "entity type correct");
    return true;
}

template<class... T1, class... T2>
inline bool assertEntityType(_EntityType<T2...> entity) {
    static_assert(checkEntityType<T1...>(entity), "entity type correct");
    return true;
}

template<class... Components>
_EntityType<Components...>& assertCastEntityType(_Entity entity, ECS* ecs) {
    assertEntityType(entity, ecs);
    return entity.castType<_EntityType<Components...>>();
}

#endif