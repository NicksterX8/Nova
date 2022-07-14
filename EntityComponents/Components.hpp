#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL2/SDL.h>
#include "../NC/cpp-vectors.hpp"
#include "../Items.hpp"
#include "../ECS/EntityType.hpp"
#include "../SECS/ECS.hpp"

namespace EC {

template<bool _Serializable = true>
struct EntityComponent {
    static constexpr bool Serializable = _Serializable;
};

#define ENTITY_TYPE_NAME_SIZE 64

struct EntityTypeEC : EntityComponent<> {
    char name[ENTITY_TYPE_NAME_SIZE] = "undefined";

    EntityTypeEC(const char* _name) {
        strcpy(this->name, _name);
    }
};

struct Grabable : EntityComponent<> {
    ::ItemStack itemGiven;
};

struct Health : EntityComponent<> {
    float healthValue;

    Health(float health) : healthValue(health) {}
};

struct Growth : EntityComponent<> {
    float growthValue;

    Growth(float growth) : growthValue(growth) {}
};

struct Position : Vec2, EntityComponent<> {
    Position(float x, float y);
    Position(Vec2 vec);
    
    Vec2 vec2() const {
        return {x, y};
    }
};

struct Size : EntityComponent<> {
    float width;
    float height;

    Size(float width, float height);

    Vec2 toVec2() const;
};

struct Render : EntityComponent<false> {
    SDL_Texture* texture;
    SDL_FRect destination;
    float rotation;
    int layer;
    Uint32 renderIndex;

    Render(SDL_Texture* texture, int layer);
};

// #define TEXTURE_NAME_SIZE 64

struct Explosion : EntityComponent<> {
    float radius;
    float damage;
    float life;
    int particleCount;

    Explosion(float radius, float damage, float life, int particleCount);
};

struct Explosive : EntityComponent<false> {
    EC::Explosion* explosion;

    Explosive(EC::Explosion* explosion);
};

#define MAX_ENTITY_NAME_LENGTH 64
#define MAX_ENTITY_TYPENAME_LENGTH 64

struct Nametag : EntityComponent<> {
    char name[MAX_ENTITY_NAME_LENGTH];
    char type[MAX_ENTITY_TYPENAME_LENGTH];

    void setName(const char* name);
    void setType(const char* type);

    Nametag();
    Nametag(const char* type, const char* name);
};

struct Motion : EntityComponent<> {
    Vec2 target;
    float speed;

    Motion(Vec2 target, float speed);
};

struct AngleMotion : EntityComponent<> {
    float rotationTarget;
    float rotationSpeed;

    AngleMotion(float rotationTarget, float rotationSpeed);
};

struct Inventory : EntityComponent<false> {
    ::Inventory inventory;

    Inventory(::Inventory inventory) : inventory(inventory) {}
};

struct Dying : EntityComponent<> {
    int timeToRemoval;

    Dying(int updatesTilRemoval) : timeToRemoval(updatesTilRemoval) {}
};

struct Inserter : EntityComponent<> {
    int cycleLength;
    int stackSize;
    int cycle;

    int reach;

    IVec2 inputTile;
    IVec2 outputTile;

    Inserter(int updatesPerMove, int stackSize, int reach, IVec2 input, IVec2 output)
    : cycleLength(updatesPerMove), stackSize(stackSize), inputTile(input), outputTile(output) {
        cycle = 0;
    }
};

struct Rotation : EntityComponent<> {
    float degrees;

    Rotation(float degrees);
};

struct Rotatable : EntityComponent<> {
    float start;
    float increment;
    // entity was rotated in the last update
    bool rotated;

    Rotatable(float start, float increment)
    : start(start), increment(increment) {
        rotated = false;
    }
};

// Entity that must have components

struct Follow : EntityComponent<> {
    EntityType<EC::Position> entity;
    float speed;

    Follow(EntityType<EC::Position> following, float speed)
    : entity(following), speed(speed) {
        
    }
};

struct ItemStack : EntityComponent<> {
    ::ItemStack item;

    ItemStack(::ItemStack itemStack) : item(itemStack) {}
};

struct Transporter : EntityComponent<> {
    float speed;

    Transporter(float speed) : speed(speed) {}
};

struct Immortal : EntityComponent<> {};

}

#endif