#ifndef ENTITY_SYSTEM_INCLUDED
#define ENTITY_SYSTEM_INCLUDED

#include "Entity.hpp"
#include "Query.hpp"
#include "EntityManager.hpp"
#include "../constants.hpp"
#include <vector>

namespace ECS {

typedef Uint32 ComponentAccessType;

namespace ComponentAccess {
    enum Type {
        None = 0,
        Read = 1,
        Write = 2,

        ReadWrite = 3,

        Add = 4,
        Remove = 8,
        Flag = 16
    };

    inline bool isWriteable(ComponentAccessType type) {
        return (type & Write);
    }

    inline bool isReadable(ComponentAccessType type) {
        return (type & Read);
    }
}

class ECS;

class EntityCommandBuffer {
    ECS* ecs;
public:
    std::vector< std::function<void(ECS*)> > commands;

    /*
    int Remove(Entity entity) {
        commands.push_back([entity](ECS* ecs){
            ecs->Remove(entity);
        });
        return 0;
    }
    */

    void DeferredStructuralChange(std::function<void(ECS*)> callback) {
        commands.push_back(callback);
    }

};

/*

class EntitySystemAccess {
    ECS* ecs;
public:
    ComponentAccessType componentAccess[NUM_COMPONENTS] = {ComponentAccess::None};
    EntitySystemAccess(ECS* ecs)
    : ecs(ecs) {
        
    }

    template<class T>
    void GiveAccess(ComponentAccessType access) {
        componentAccess[getID<T>()] = (ComponentAccessType)(componentAccess[getID<T>()] | access);
    }

    template<class T>
    inline T* GetReadWrite(Entity entity) {
#ifdef DEBUG
        ComponentAccessType type = componentAccess[getID<T>()];
        if (!ComponentAccess::isReadable(type) || !ComponentAccess::isWriteable(type)) {
            LogError("Attempted to get read write access to a component without the proper type. Returning NULL.");
            return NULL;
        }
#endif
        return ecs->Get<T>(entity);
    }

    template<class T>
    inline const T* GetReadOnly(Entity entity) {
#ifdef DEBUG
        ComponentAccessType type = componentAccess[getID<T>()];
        if (!ComponentAccess::isReadable(type)) {
            LogError("Attempted to get read only access to a component without the proper type! Returning NULL.");
            return NULL;
        }
#endif
        return ecs->Get<T>(entity);
    }

    template<class T>
    inline bool EntityHasComponent(Entity entity) const {
#ifdef DEBUG
        ComponentAccessType type = componentAccess[getID<T>()];
        if (type == ComponentAccess::None) {
            LogError("Attempted to check a component without access to that component! Returning NULL.");
        }
#endif
        return ecs->entityComponents(entity.id)[getID<T>()];
    }

    inline bool EntityExists(Entity entity) const {
        return ecs->EntityExists(entity);
    }

    inline ComponentFlags entityComponents(EntityID id) const {
        return ecs->entityComponents(id);
    }

    template<class... Components>
    inline bool EntityHas(Entity entity) const {
        constexpr ComponentFlags signature = componentSignature<Components...>();
        return (ecs->entityComponents(entity.id) & signature) == signature;
    }

    template<class T>
    inline void ScheduleRemoveComponent(Entity entity) const {
#ifdef DEBUG
        ComponentAccessType type = componentAccess[getID<T>()];
        if (!(ComponentAccess::Remove & type)) {
            LogError("Attempted to schedule remove a component without remove access to that component! Returning NULL.");
            return;
        }
#endif
        ecs->ScheduleRemoveComponents<T>(entity);
    }
};

#define DECL_ON_UPDATE void OnUpdate(EntityManager* em) {\
        for (Uint32 e = 0; e < nEntities; e++) {\
            Entity entity = entities[e];\
            Update(em, entity);\
        }\
    }

typedef std::function<void()> EntitySystemUpdateLambda;
using EntitySystemUpdate = void(*)();

class EntitySystem : public EntitySystemInterface {
public:
    EntitySystemAccess sys;
    EntityCommandBuffer* commandBuffer = NULL;

    EntitySystem(ECS* ecs) : EntitySystemInterface(), sys(ecs) {
        commandBuffer = new EntityCommandBuffer();
    }

    EntitySystem(ECS* ecs, Uint32 reserve) : EntitySystemInterface(reserve), sys(ecs) {
        commandBuffer = new EntityCommandBuffer();
    }

    virtual void Update() = 0;

protected:

    void DestroyEntity(Entity entity) {
        commandBuffer->DeferredStructuralChange([entity](ECS* ecs){
            ecs->Destroy(entity);
        });
    }

    template<class... Components>
    using EntityCallback = std::function<void(EntityType<Components...>)>;

    template<class Query>
    inline void QueryForEach(std::function<void(Entity entity)> callback) {
        for (int e = nEntities-1; e >= 0; e--) {
            if (!sys.EntityExists(entities[e])) {
                RemoveEntityLocally(e);
                continue;
            }

            if (!Query::check(sys.entityComponents(entities[e].id))) {
                RemoveEntityLocally(e);
                continue;
            }
            
            callback(entities[e]);
        }
        
    }

    inline void ForEach(ECS* ecs, std::function<void(Entity)> callback) {
        for (int e = nEntities-1; e >= 0; e--) {
            if (!ecs->EntityExists(entities[e])) {
                RemoveEntityLocally(e);
                continue;
            }

            if (!Query(ecs->entityComponents(entities[e].id))) {
                RemoveEntityLocally(e);
                continue;
            }
            
            callback(entities[e]);
        }
    }

    template<class T>
    using Func = std::function<void(T)>;

    template<class... Components, class Func>
    inline void ForEach(Func callback) {

        for (int e = nEntities-1; e >= 0; e--) {
            if (!sys.EntityExists(entities[e])) {
                RemoveEntityLocally(e);
                continue;
            }

            if (!Query(sys.entityComponents(entities[e].id))) {
                RemoveEntityLocally(e);
                continue;
            }
            
            if ((sys.entityComponents(entities[e].id) & componentSignature<Components...>()) == componentSignature<Components...>())
                callback(entities[e].cast<Components...>());
        }
    }
};

*/

}

#endif