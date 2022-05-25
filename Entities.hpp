#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS.hpp"
#include "Components.hpp"

namespace Entities {

    Entity Explosion(ECS* ecs, float radius, float damage);

    Entity Explosive(ECS* ecs, Vec2 position, Vec2 size, SDL_Texture* texture, ExplosionComponent* explosion);

    void throwExplosive(ECS* ecs, Entity explosive, Vec2 target, float speed);

    Entity Grenade(ECS* ecs, Vec2 position, Vec2 size);
    Entity Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target);
    Entity ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target);
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size);

    Entity Chest(ECS* ecs, Vec2 position, int size, int width, int height);

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, RenderComponent render, MotionComponent motion);

    Entity Random(ECS* ecs);

}

#endif