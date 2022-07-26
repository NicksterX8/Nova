#ifndef SCECS_ENTITY_INCLUDED
#define SCECS_ENTITY_INCLUDED

#include "../ECS/EntityType.hpp"

using ECS::ComponentFlags;

using ECS::EntityID;
using ECS::EntityVersion;

template<class... Components>
using EntityT = ECS::EntityType<Components...>;
using Entity = EntityT<>;

using ECS::NullEntity;

using ECS::OptionalEntity;
using ECS::OptionalEntityT;

#endif