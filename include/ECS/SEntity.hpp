#ifndef SCECS_ENTITY_INCLUDED
#define SCECS_ENTITY_INCLUDED

#include "../ECS/Entity.hpp"

using ECS::ComponentFlags;
using ECS::NullComponentID;

using ECS::EntityID;
using ECS::EntityVersion;

template<class... Components>
using EntityT = ECS::EntityType<Components...>;
using Entity = EntityT<>;

using ECS::NullEntity;

template<class T>
struct OptionalEntityT : public T {
    using T::T;

    OptionalEntityT() : T(NullEntity.castType<T>()) {}

    OptionalEntityT(T entity) : T(entity) {}
};

template<class... Components>
using OptionalEntity = EntityT<Components...>;

#endif