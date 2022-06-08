#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL2/SDL.h>
#include "../NC/cpp-vectors.hpp"
#include "../Items.hpp"
#include "EntityType.hpp"

struct Tile;

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
    float rotation;
    int layer;

    RenderComponent(SDL_Texture* texture, int layer);
};

// Render with the position representing the center of the entity, not the top left
struct CenteredRenderFlagComponent {};

struct ExplosionComponent {
    float radius;
    float damage;
    float life;
    int particleCount;

    ExplosionComponent(float radius, float damage, float life, int particleCount);
};

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

struct AngleMotionEC {
    float rotationTarget;
    float rotationSpeed;

    AngleMotionEC(float rotationTarget, float rotationSpeed);
};

struct InventoryComponent {
    Inventory inventory;
};

struct DyingComponent {
    int timeToRemoval;
};

struct InserterComponent {
    int cycleLength;
    int stackSize;
    int cycle;

    int reach;

    Tile* inputTile;
    Tile* outputTile;

    InserterComponent(int updatesPerMove, int stackSize, int reach, Tile* input, Tile* output)
    : cycleLength(updatesPerMove), stackSize(stackSize), inputTile(input), outputTile(output) {
        cycle = 0;
    }
};

struct RotationComponent {
    float degrees;
};

struct RotatableComponent {
    float start;
    float increment;
    // entity was rotated in the last update
    bool rotated;

    RotatableComponent(float start, float increment)
    : start(start), increment(increment) {
        rotated = false;
    }
};

// Entity that must have components

struct FollowComponent {
    EntityType<PositionComponent> entity;
    float speed;

    FollowComponent(EntityType<PositionComponent> following, float speed)
    : entity(following), speed(speed) {
        
    }
};

struct ItemStackComponent {
    ItemStack item;
};

struct TransporterEC {
    float speed;
};

struct ImmortalEC {};

#endif