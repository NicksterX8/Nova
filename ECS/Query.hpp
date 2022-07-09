#ifndef ENTITY_QUERY_INCLUDED
#define ENTITY_QUERY_INCLUDED

#include "Entity.hpp"
#include "ECS.hpp"
#include <array>

template<class... Components>
struct ComponentList {
    constexpr static ComponentFlags signature() {
        return componentSignature<Components...>();
    }

    constexpr static size_t count() {
        return sizeof...(Components);
    }

    using PoolArrayType = std::array<ComponentPool*, sizeof...(Components)>;

    inline static PoolArrayType getPoolArray(const ECS* ecs) {
        ComponentPool* pools[] = {0, ((void)0, ecs->manager.getPool<Components>()) ...};
        PoolArrayType poolArray;
        for (Uint32 i = 0; i < sizeof(pools) / sizeof(ComponentPool*) - 1; i++) {
            poolArray[i] = pools[i+1];
        }
        return poolArray;
    }
};

template<class... Components>
constexpr auto componentListPoolSizes(const ECS* ecs) {
    Uint32 sizes[] = {0, ((void)0, ecs->componentPoolSize<Components>()) ...};

    std::array<Uint32, sizeof...(Components)> result;
    for (size_t i = 0; i < sizeof...(Components); i++) {
        result[i] = sizes[i+1];
    }
    return result;
}

// @param outID Pass a valid address to get filled in with the id of the component pool with the minimum size
template<class... Components>
Uint32 minimumPoolSize(const ECS* ecs, ComponentID* outID) {
    if (sizeof...(Components) == 0) return 0;

    constexpr ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
    auto componentPoolSizes = componentListPoolSizes<Components...>(ecs);
    Uint32 minComponentPoolSize = componentPoolSizes[0];
    ComponentID minComponentPoolId = ids[1];
    // array starts at index one due to dummy 0 used in parameter unpacking
    for (size_t i = 1; i < sizeof(ids) / sizeof(ComponentID); i++) {
        if (componentPoolSizes[i] < minComponentPoolSize) {
            minComponentPoolSize = componentPoolSizes[i];
            minComponentPoolId = ids[i];
        }
    }

    if (outID) *outID = minComponentPoolId;
    return minComponentPoolSize;
}

template<class... Components>
constexpr ComponentPool* smallestComponentPool(const ECS* ecs) {
    if (sizeof...(Components) == 0) return NULL;

    constexpr ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
    ComponentPool* pools[] = {0, ((void)0, ecs->manager.getPool<Components>()) ...};
    Uint32 minSize = pools[1];
    ComponentPool* minSizePool = pools[1];
    for (Uint32 i = 2; i < sizeof(pools) / sizeof(ComponentPool*); i++) {
        Uint32 poolSize = pools[i]->size();
        if (poolSize < minSize) {
            minSize = poolSize;
            minSizePool = pools[i];
        }
    }

    return minSizePool;
}


template<class... Components>
using RequireComponents = ComponentList<Components...>;

template<class... Components>
using AvoidComponents = ComponentList<Components...>;

template<class... Components>
using LogicalOrComponentSet = ComponentList<Components...>;

template<class... Sets>
struct LogicalOrComponents {};

//template<class... Sets>
//using LogicalORComponents = LogicalOrComponentSet<Components...>;

template<class Require, class Avoid, class LogicalORs>
struct EntityQuery {
    using Self = EntityQuery<Require, Avoid, LogicalORs>;
    static constexpr Require require = {};
    static constexpr Avoid avoid = {};
    //static constexpr size_t NumLogicalORs = sizeof...(LogicalORs);
    static constexpr LogicalORs logicalORs = {};
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
    
    //static constexpr ComponentFlags logicalOrSignatures[NumLogicalORs] = {0, ((void)0, listSignature<LogicalORs>()) ...};
    // LogicalOrList logicalORs;
    // std::array<LogicalOrComponentSet, NumLogicalOrSets> logicalORs;

    /*
    static constexpr ComponentFlags requiredSignature() {
        return listSignature(require);
    }

    static constexpr ComponentFlags avoidedSignature() {
        return listSignature(avoid);
    }
    */

    static constexpr bool check(ComponentFlags signature) {
        const size_t numLogicalORs = getNumLogicalORs(logicalORs);

        for (size_t i = 0; i < numLogicalORs; i++) {
            ComponentFlags logicalOrSignature = getLogicalOrSignature(logicalORs, i);
            if (signature.hasNone(logicalOrSignature)) {
                return false;
            }
        }

        return (signature.hasAll(Require::signature()) && signature.hasNone(Avoid::signature()));
    }

private:
    template<class... RequiredComponents, class... AvoidedComponents, class... LogicalOrComponentSets>
    static void ForEachExpanded(const ECS* ecs, 
        const EntityQuery<
            RequireComponents<RequiredComponents...>,
            AvoidComponents<AvoidedComponents...>,
            LogicalOrComponents<LogicalOrComponentSets...>
        > query, std::function<void(Entity)> callback)
    {
        //constexpr ComponentID requiredIDs[] = {0, ((void)0, getID<RequiredComponents>()) ...};
        //constexpr ComponentID avoidedIDs[] = {0, ((void)0, getID<AvoidedComponents>()) ...};

        //const size_t numRequiredComponents = getNumRequiredComponents(require);
        //const size_t numAvoidedComponents = getNumAvoidedComponents(avoid);

        if (Require::count() > 0) {
            // find minimum component pool size so we can search through that one, for speed
            // Require::PoolArrayType requiredPools = Require::getPoolArray();
            ComponentPool* smallestPool = smallestComponentPool<RequiredComponents...>(ecs);
            for (int c = smallestPool->size()-1; c >= 0; c--) {
                EntityID entityID = smallestPool->componentOwners[c];
                ComponentFlags entityComponents = ecs->entityComponents(entityID);
                if (entityComponents.hasAll(Require::signature())) {

                }
            }
        }

    }
public:

    static void ForEach(const ECS* ecs, std::function<void(Entity)> callback) {
        constexpr Self self = {};
        ForEachExpanded(ecs, self, callback);
    };

    constexpr bool check_t(ComponentFlags signature) const {
        return check(signature);
    }
};



#endif