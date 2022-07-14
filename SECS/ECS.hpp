#ifndef SECS_ECS_INCLUDED
#define SECS_ECS_INCLUDED

#include "../ECS/ECS.hpp"
#include "../ECS/EntityType.hpp"
#include "../ECS/Query.hpp"

extern const char* componentNames[NUM_COMPONENTS];

using ECS::ComponentFlags;
using ECS::Entity;
using ECS::EntityID;
using ECS::EntityVersion;
using ECS::NullEntity;
using ECS::EntityType;
using ECS::OptionalEntity;
using ECS::OptionalEntityT;
using ECS::EntityQuery;

#endif