#ifndef ENTITY_QUERY_INCLUDED
#define ENTITY_QUERY_INCLUDED

#include "Entity.hpp"
#include "ECS.hpp"
#include <array>

namespace ECS {

template<class... Types>
struct TypeList {
    constexpr static size_t size() {
        return sizeof...(Types);
    }
};

template<class... Components>
struct ComponentList : public TypeList<Components...> {
    constexpr static ComponentFlags signature() {
        return componentSignature<Components...>();
    }
};

template<class... Components>
constexpr ComponentPool* smallestComponentPool(const EntityManager* em) {
    if (sizeof...(Components) == 0) return NULL;

    constexpr ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
    ComponentPool* pools[] = {0, ((void)0, em->getPool<Components>()) ...};
    Uint32 minSize = pools[1]->size;
    ComponentPool* minSizePool = pools[1];
    for (Uint32 i = 2; i < sizeof(pools) / sizeof(ComponentPool*); i++) {
        Uint32 poolSize = pools[i]->size;
        if (poolSize < minSize) {
            minSize = poolSize;
            minSizePool = pools[i];
        }
    }

    return minSizePool;
}


template<class... Components>
class RequireComponents : public ComponentList<Components...> {
public:
    static inline ComponentPool* smallestPool(const EntityManager* em) {
        return smallestComponentPool<Components...>(em);
    }
};

template<class... Components>
using AvoidComponents = ComponentList<Components...>;

template<class... Components>
using LogicalOrComponentSet = ComponentList<Components...>;

template<class... Sets>
struct LogicalOrComponents : public TypeList<Sets...> {};

template<class Require, class Avoid=AvoidComponents<>, class LogicalORs=LogicalOrComponents<>>
struct EntityQuery {
    using Self = EntityQuery<Require, Avoid, LogicalORs>;
    static constexpr Require require = {};
    using Required = Require;
    static constexpr Avoid avoid = {};
    using Avoided = Avoid;
    //static constexpr size_t NumLogicalORs = sizeof...(LogicalORs);
    static constexpr LogicalORs logicalORs = {};
    using LogicalORed = LogicalORs;
private:
    template<class... Components>
    static constexpr ComponentFlags setSignature(LogicalOrComponentSet<Components...> set) {
        return componentSignature<Components...>();
    }

    template<class Set>
    static constexpr Set makeSet() {
        constexpr Set set;
        return set;
    }

public:

    template<class... Sets>
    static constexpr size_t getNumLogicalORs(LogicalOrComponents<Sets...> _logicalORs) {
        return sizeof...(Sets);
    }

    template<class... Components>
    static constexpr size_t getNumRequiredComponents(RequireComponents<Components...> requiredComponents) {
        return sizeof...(Components);
    }

    template<class... Components>
    static constexpr size_t getNumAvoidedComponents(AvoidComponents<Components...> avoidedComponents) {
        return sizeof...(Components);
    }

    template<class... Sets>
    static constexpr ComponentFlags getLogicalOrSignature(LogicalOrComponents<Sets...> _logicalORs, int index) {
        constexpr ComponentFlags setSignatures[] = {0, ((void)0, Sets::signature()) ...};

        return setSignatures[index+1];
    }

    static constexpr bool Check(ComponentFlags signature) {
        const size_t numLogicalORs = getNumLogicalORs(logicalORs);

        for (size_t i = 0; i < numLogicalORs; i++) {
            ComponentFlags logicalOrSignature = getLogicalOrSignature(logicalORs, i);
            if (signature.hasNone(logicalOrSignature)) {
                return false;
            }
        }

        return (signature.hasAll(Require::signature()) && signature.hasNone(Avoid::signature()));
    }
public:
    static void ForEach(const EntityManager* em, const std::function<void(Entity)>& callback) {
        if (Require::size() > 0) {
            // find minimum component pool size so we can search through that one, for speed
            //Require::PoolArrayType requiredPools = Require::getPoolArray();
            //ComponentPool* iteratedPool = smallestComponentPool<RequiredComponents...>(ecs);
            ComponentPool* iteratedPool = Require::smallestPool(em);
            for (int c = iteratedPool->size-1; c >= 0; c--) {
                EntityID entityID = iteratedPool->componentOwners[c];
                EntityData entityData = em->entityDataList[entityID];
                // check required and avoided
                if (entityData.flags.hasAll(Require::signature()) && entityData.flags.hasNone(Avoid::signature())) {
                    // check logical ORs
                    for (size_t i = 0; i < LogicalORs::size(); i++) {
                        const ComponentFlags logicalOrSignature = getLogicalOrSignature(logicalORs, i);
                        if (entityData.flags.hasNone(logicalOrSignature)) {
                            goto endEntityLoop;
                        }
                    }

                    // Entity has required, does not have avoided, and has the logical OR components.
                    callback(Entity(entityID, entityData.version));
                }
            endEntityLoop:
                continue;
            }
        } else {
            // iterate main entity list
            const Entity* entities = (Entity*)em->entities;
            for (int c = em->getEntityCount()-1; c >= 0; c--) {
                Entity entity = entities[c];
                ComponentFlags entityComponents = em->EntitySignature(entity.id);
                if (entityComponents.hasNone(Avoid::signature())) {
                    for (size_t i = 0; i < LogicalORs::size(); i++) {
                        const ComponentFlags logicalOrSignature = getLogicalOrSignature(logicalORs, i);
                        if (entityComponents.hasNone(logicalOrSignature)) {
                            goto endEntityLoop2;
                        }
                    }

                    callback(entity);
                }
            endEntityLoop2:
                continue;
            }
            
        }
    }

    constexpr bool check(ComponentFlags signature) const {
        return Check(signature);
    }
};

}

#endif