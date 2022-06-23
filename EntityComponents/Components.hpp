#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL2/SDL.h>
#include "../NC/cpp-vectors.hpp"
#include "../Items.hpp"
#include "../ECS/EntityType.hpp"

template<bool _Serializable = true>
struct EntityComponent {
    static constexpr bool Serializable = _Serializable;
};

#define ENTITY_TYPE_NAME_SIZE 64

struct EntityTypeComponent : EntityComponent<> {
    char name[ENTITY_TYPE_NAME_SIZE];

    EntityTypeComponent(const char* name) {
        strncpy(this->name, name, ENTITY_TYPE_NAME_SIZE);
    }
};

struct GrabableEC : EntityComponent<> {
    ItemStack itemGiven;
};

struct HealthComponent : EntityComponent<> {
    float healthValue;

    HealthComponent(float health) : healthValue(health) {}
};

struct GrowthComponent : EntityComponent<> {
    float growthValue;

    GrowthComponent(float growth) : growthValue(growth) {}
};

struct PositionComponent : Vec2, EntityComponent<> {
    PositionComponent(float x, float y);
    PositionComponent(Vec2 vec);
    
    Vec2 vec2() {
        return {x, y};
    }
};

struct SizeComponent : EntityComponent<> {
    float width;
    float height;

    SizeComponent(float width, float height);

    Vec2 toVec2() const;
};

struct RenderComponent : EntityComponent<false> {
    SDL_Texture* texture;
    SDL_FRect destination;
    float rotation;
    int layer;

    RenderComponent(SDL_Texture* texture, int layer);
};

// #define TEXTURE_NAME_SIZE 64

// Render with the position representing the center of the entity, not the top left
struct CenteredRenderFlagComponent : EntityComponent<> {};

struct ExplosionComponent : EntityComponent<> {
    float radius;
    float damage;
    float life;
    int particleCount;

    ExplosionComponent(float radius, float damage, float life, int particleCount);
};

struct ExplosiveComponent : EntityComponent<false> {
    ExplosionComponent* explosion;

    ExplosiveComponent(ExplosionComponent* explosion);
};

#define MAX_ENTITY_NAME_LENGTH 64
#define MAX_ENTITY_TYPENAME_LENGTH 64

struct NametagComponent : EntityComponent<> {
    char name[MAX_ENTITY_NAME_LENGTH];
    char type[MAX_ENTITY_TYPENAME_LENGTH];

    void setName(const char* name);
    void setType(const char* type);

    NametagComponent();
    NametagComponent(const char* type, const char* name);
};

struct MotionComponent : EntityComponent<> {
    Vec2 target;
    float speed;

    MotionComponent(Vec2 target, float speed);
};

struct AngleMotionEC : EntityComponent<> {
    float rotationTarget;
    float rotationSpeed;

    AngleMotionEC(float rotationTarget, float rotationSpeed);
};

struct InventoryComponent : EntityComponent<false> {
    Inventory inventory;

    InventoryComponent(Inventory inventory) : inventory(inventory) {}
};

struct DyingComponent : EntityComponent<> {
    int timeToRemoval;

    DyingComponent(int updatesTilRemoval) : timeToRemoval(updatesTilRemoval) {}
};

struct InserterComponent : EntityComponent<> {
    int cycleLength;
    int stackSize;
    int cycle;

    int reach;

    IVec2 inputTile;
    IVec2 outputTile;

    InserterComponent(int updatesPerMove, int stackSize, int reach, IVec2 input, IVec2 output)
    : cycleLength(updatesPerMove), stackSize(stackSize), inputTile(input), outputTile(output) {
        cycle = 0;
    }
};

struct RotationComponent : EntityComponent<> {
    float degrees;

    RotationComponent(float degrees);
};

struct RotatableComponent : EntityComponent<> {
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

struct FollowComponent : EntityComponent<> {
    EntityType<PositionComponent> entity;
    float speed;

    FollowComponent(EntityType<PositionComponent> following, float speed)
    : entity(following), speed(speed) {
        
    }
};

struct ItemStackComponent : EntityComponent<> {
    ItemStack item;

    ItemStackComponent(ItemStack itemStack) : item(itemStack) {}
};

struct TransporterEC : EntityComponent<> {
    float speed;

    TransporterEC(float speed) : speed(speed) {}
};

struct ImmortalEC : EntityComponent<> {};

#endif