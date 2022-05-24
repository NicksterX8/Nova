#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL2/SDL.h>
#include "NC/cpp-vectors.hpp"

extern Uint32 componentCount;

struct Component {};

#define COMPONENTS \
    PositionComponent, SizeComponent, RenderComponent, \
    HealthComponent, GrowthComponent, CenteredRenderFlagComponent, \
    ExplosionComponent, NametagComponent, ExplosiveComponent, \
    MotionComponent

struct HealthComponent {
    float healthValue;
};

struct GrowthComponent {
    float growthValue;
};

struct PositionComponent : Vec2 {
    PositionComponent(float x, float y);
    PositionComponent(Vec2 vec);
};

struct SizeComponent {
    float width;
    float height;
};

struct RenderComponent {
    SDL_Texture* texture;
    SDL_FRect destination;
    int layer;

    RenderComponent(SDL_Texture* texture, int layer);
};

// Render with the position representing the center of the entity, not the top left
struct CenteredRenderFlagComponent {};

struct ExplosionComponent {
    float radius;
    float damage;
    float life;

    ExplosionComponent(float radius, float damage, float life);
};

extern ExplosionComponent grenadeExplosion;

struct ExplosiveComponent {
    ExplosionComponent* explosion;

    ExplosiveComponent(ExplosionComponent* explosion);
};

struct NametagComponent {
    char name[64];
    char type[64];
};

struct MotionComponent {
    Vec2 target;
    float speed;

    MotionComponent(Vec2 target, float speed);
};

#endif