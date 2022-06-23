#ifndef ENTITY_QUERY_INCLUDED
#define ENTITY_QUERY_INCLUDED

#include "Entity.hpp"
#include "ECS.hpp"
#include <array>

template<class... Components>
struct ComponentList {};

template<class... Components>
constexpr ComponentFlags listSignature(ComponentList<Components...> list) {
    return componentSignature<Components...>();
}

template<class T>
struct ComponentPoolSizePair {
    ComponentPool<T>* pool;
    Uint32 size;
};

template<class C1, class C2>
Uint32 minSize(const ComponentPool<C1>* pool1, const ComponentPool<C2>* pool2) {
    Uint32 size1 = pool1->getSize();
    Uint32 size2 = pool2->getSize();

    return size1;
}

template<class... Components>
constexpr auto componentListPoolSizes(const ECS* ecs) {
    Uint32 sizes[] = {0, ((void)0, ecs->componentPoolSize<Components>()) ...};

    std::array<Uint32, sizeof...(Components)> result;
    for (size_t i = 0; i < sizeof...(Components); i++) {
        result[i] = sizes[i+1];
    }
    return result;
}

template<class... Components>
constexpr auto componentListPoolSizes(const ECS* ecs, ComponentList<Components...> list) {
    return componentListPoolSizes<Components...>(ecs);
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

/*
template<class T>
auto maybeGetPool(const ECS* ecs, ComponentID id) {
    if (getID<T>() == id) {
        return ecs->manager.getPool<T>();
    }

    
}
*/

/*
template<class... Components>
auto getPoolFromID(ComponentID id) {
    constexpr ComponentID ids[] = {0, ((void)0, getID<Components>()) ...};
    for (size_t i = 1; i < sizeof(ids) / sizeof(ComponentID); i++) {
        if (ids[i] == id) {
            return 
        }
    }
}
*/

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
        constexpr ComponentFlags setSignatures[] = {0, ((void)0, setSignature(makeSet<Sets>())) ...};
        return setSignatures[index+1];
    }
    
    //static constexpr ComponentFlags logicalOrSignatures[NumLogicalORs] = {0, ((void)0, listSignature<LogicalORs>()) ...};
    // LogicalOrList logicalORs;
    // std::array<LogicalOrComponentSet, NumLogicalOrSets> logicalORs;

    static constexpr ComponentFlags requiredSignature() {
        return listSignature(require);
    }

    static constexpr ComponentFlags avoidedSignature() {
        return listSignature(avoid);
    }

    static constexpr bool check(ComponentFlags signature) {
        const size_t numLogicalORs = getNumLogicalORs(logicalORs);

        for (size_t i = 0; i < numLogicalORs; i++) {
            ComponentFlags logicalOrSignature = getLogicalOrSignature(logicalORs, i);
            if (!(signature & logicalOrSignature).any()) {
                return false;
            }
        }

        return ((signature & requiredSignature()) == requiredSignature()) &&
            !(signature & avoidedSignature()).any();
    }

private:
    template<class... RequiredComponents, class... AvoidedComponents, class... LogicalOrComponentSets>
    static void ForEachExpanded(const ECS* ecs, 
        EntityQuery<
            RequireComponents<RequiredComponents...>,
            AvoidComponents<AvoidedComponents...>,
            LogicalOrComponents<LogicalOrComponentSets...>
        > query, std::function<void(Entity)> callback)
    {
        constexpr ComponentID requiredIDs[] = {0, ((void)0, getID<RequiredComponents>()) ...};
        constexpr ComponentID avoidedIDs[] = {0, ((void)0, getID<AvoidedComponents>()) ...};

        const size_t numRequiredComponents = getNumRequiredComponents(require);
        const size_t numAvoidedComponents = getNumAvoidedComponents(avoid);
        if (numRequiredComponents > 0) {
            // find minimum component pool size so we can search through that one
            ComponentID minID;
            Uint32 minSize = minimumPoolSize<RequiredComponents...>(ecs, &minID);
            // auto startPool = getPoolFromID<RequiredComponents...>(minID);

            // for (Uint32 e = 0; e < minSize; )

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