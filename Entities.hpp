#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS.hpp"
#include "Components.hpp"

namespace Entities {

    Entity Explosion(ECS* ecs, float radius, float damage);

    Entity Grenade(ECS* ecs, Vec2 position, Vec2 size);

    Entity ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target);
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size);

    Entity Random(ECS* ecs);

}

#endif