#ifndef ENTITY_SYSTEM_INCLUDED
#define ENTITY_SYSTEM_INCLUDED

#include "Entity.hpp"
#include "EntityManager.hpp"
#include "EntityType.hpp"

#define DECL_ON_UPDATE void OnUpdate(EntityManager* em) {\
        for (Uint32 e = 0; e < nEntities; e++) {\
            Entity entity = entities[e];\
            Update(em, entity);\
        }\
    }

class EntitySystem : public EntitySystemInterface {
public:
    EntitySystem() : EntitySystemInterface() {}

    EntitySystem(Uint32 reserve) : EntitySystemInterface(reserve) {}

protected:
    template<class... Components>
    using EntityCallback = std::function<void(EntityType<Components...>)>;

    inline void ForEach(std::function<void(Entity)> callback) {
        for (Uint32 e = 0; e < nEntities; e++) {
            callback(entities[e]);
        }
    }

    template<class T>
    using Func = std::function<void(T)>;

    template<class... Components, class Func>
    inline void ForEach(ECS* ecs, Func callback) {
        for (Uint32 e = 0; e < nEntities; e++) {
            if ((ecs->entityComponents(entities[e].id) & componentSignature<Components...>()) == componentSignature<Components...>())
                callback(entities[e].castType<EntityType<Components...>>());
        }
    }
};

#endif