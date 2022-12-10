#ifndef SECS_ECS_INCLUDED
#define SECS_ECS_INCLUDED

#include "ECS/ECS.hpp"
#include "ECS/Query.hpp"
#include "Entity.hpp"

extern const char* componentNames[ECS_NUM_COMPONENTS];

using EC_ID = ECS::ComponentID;

using ECS::EntityQuery;

#endif