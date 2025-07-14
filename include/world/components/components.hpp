#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL3/SDL.h>
#include "utils/vectors_and_rects.hpp"
#include "items/items.hpp"
#include "ECS/Entity.hpp"
#include "ECS/componentMacros.hpp"
#include "rendering/text/text.hpp"

#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; \
    static_assert(true | ComponentIDs::name, "Checking for id"); \
    static_assert(std::is_trivially_copy_constructible_v<name>, "Components must be trivially copyable"); \
    static_assert(std::is_trivially_move_constructible_v<name>, "Components must be trivially moveable"); \
    static_assert(std::is_trivially_destructible_v<name>, "Components must be trivially assignable");

struct RawArray {
    const void* data;
    const size_t size;

    RawArray(const void* ptr, size_t size) : data(ptr), size(size) {}

    template<class T>
    RawArray(const T& value) : data(&value), size(sizeof(value)) {}
};

namespace World {
 
namespace EC {

using ComponentID = ECS::ComponentID;

namespace ComponentIDs {
    #define WORLD_REGULAR_COMPONENT_LIST \
        Grabbable, Nametag, Health, CollisionBox, ViewBox,  \
        Size, Position, Inventory, Motion, AngleMotion,     \
        Render, Inserter, Dynamic, Immortal, Selected, Follow, \
        Special, TransportLineEC, Transporter, Dying, Fresh, \
        ItemStack, Rotation, Rotatable, Explosion, \
        Explosive, Growth, Point, Text
    #define WORLD_PROTOTYPE_COMPONENT_LIST \
        Hostile, Nature, EntityRenderLayer, Living
    #define WORLD_COMPONENT_LIST WORLD_REGULAR_COMPONENT_LIST COMMA WORLD_PROTOTYPE_COMPONENT_LIST
    GEN_IDS(ComponentIDs, ComponentID, WORLD_COMPONENT_LIST, Count)
};

using ECS::Entity;

BEGIN_COMPONENT(Grabbable)
    ::ItemStack itemGiven;

    Grabbable(::ItemStack itemGiven) : itemGiven(itemGiven) {}
END_COMPONENT(Grabbable)

BEGIN_COMPONENT(Health)
    float health;
    float damageTaken = 0.0f;
    Tick timeDamaged = NullTick;
    int32_t iFrames = 0; // frames of invincibility the entity has. -1 for permanent invincibility

    #define ENTITY_HIT_COOLDOWN 30 // frames

    void damage(float amount) {
        if (!iFrames) {
            health -= amount;
            damageTaken = amount;
            timeDamaged = Metadata->getTick();
            iFrames = ENTITY_HIT_COOLDOWN;
        }
    }

    Health(float Health) : health(Health) {}
END_COMPONENT(Health)

BEGIN_COMPONENT(Growth)
    float growth;

    Growth(float Growth) : growth(Growth) {}
END_COMPONENT(Growth)

BEGIN_COMPONENT(ViewBox)
    Box box;

    constexpr ViewBox(Box box) : box(box) {}

    constexpr ViewBox(Vec2 min, Vec2 size) : box(Box{min, size}) {}

    static constexpr ViewBox BottomLeft(Vec2 size) {
        return ViewBox(Vec2(0.0, 0.0), size);
    }

    static constexpr ViewBox Centered(Vec2 size) {
        return ViewBox{Box{size/2.0f, size}};
    }
        
END_COMPONENT(ViewBox)

BEGIN_COMPONENT(CollisionBox)
    Box box;

    CollisionBox(Box box) : box(box) {}

    CollisionBox(Vec2 min, Vec2 size) {
        box = Box{min, size};
    }
END_COMPONENT(CollisionBox)

BEGIN_COMPONENT(Point)
    Vec2 p;

    Point(Vec2 p) : p(p) {}
END_COMPONENT(Point)

BEGIN_COMPONENT(Position)
    float x;
    float y;

    Position(float x, float y) : x(x), y(y) {}
    Position(Vec2 vec) : x(vec.x), y(vec.y) {}
    
    Vec2 vec2() const {
        return Vec2(x, y);
    }
END_COMPONENT(Position)

BEGIN_COMPONENT(Size)
    float width;
    float height;

    Size(float width, float height) : width(width), height(height) {}
    Size(Vec2 vec) : width(vec.x), height(vec.y) {}

    Vec2 vec2() const {
        return Vec2(width, height);
    };
END_COMPONENT(Size)

#define RENDER_COMPONENT_MAX_TEX 3
BEGIN_COMPONENT(Render)
    static constexpr Box FullBox = Box{Vec2(0), Vec2(1)};

    struct Texture {
        TextureID tex;
        int layer;
        Box box;
        float opacity;

        Texture() {}

        Texture(TextureID tex, int layer, Box box = FullBox, float opacity = 1.0f) : tex(tex), layer(layer), box(box), opacity(opacity) {}
        Texture(TextureID tex, int layer, float opacity) : tex(tex), layer(layer), box(FullBox), opacity(opacity) {}
    };
    Texture textures[RENDER_COMPONENT_MAX_TEX];
    int numTextures;

    // constructor for single texture that takes up whole viewbox
    Render(TextureID texture, int layer, float opacity = 1.0f) {
        textures[0] = {texture, layer, FullBox, opacity};
        numTextures = 1;
    }

    Render(Texture* textures, int numTextures) {
        this->numTextures = numTextures;
        for (int i = 0; i < numTextures; i++) {
            this->textures[i] = textures[i];
        }
    }
END_COMPONENT(Render)

BEGIN_COMPONENT(Text)
    const char* message;
    TextRenderingSettings rendering;
    TextFormattingSettings formatting;
    Box box;
END_COMPONENT(Text)

BEGIN_COMPONENT(Explosion)
    float radius;
    float damage;
    float life;
    int particleCount;

    Explosion(float radius, float damage, float life, int particleCount);
END_COMPONENT(Explosion)

BEGIN_COMPONENT(Explosive)
    Explosion explosion;

    Explosive(const Explosion& explosion) : explosion(explosion) {

    }
END_COMPONENT(Explosive)


#define MAX_ENTITY_NAMETAG_LENGTH 64

BEGIN_COMPONENT(Nametag)
    char name[MAX_ENTITY_NAMETAG_LENGTH];

    void setName(const char* name);

    Nametag() {
        name[0] = '\0';
    }
    Nametag(const char* name) {
        setName(name);
    }
END_COMPONENT(Nametag)

BEGIN_COMPONENT(Motion)
    Vec2 target;
    float speed;

    Motion(Vec2 target, float speed) : target(target), speed(speed) {}
END_COMPONENT(Motion)

BEGIN_COMPONENT(AngleMotion)
    float rotationTarget;
    float rotationSpeed;

    AngleMotion(float rotationTarget, float rotationSpeed) : rotationTarget(rotationTarget), rotationSpeed(rotationSpeed) {}
END_COMPONENT(AngleMotion)

BEGIN_COMPONENT(Inventory)
    ::Inventory inventory;

    Inventory(::Inventory inventory) : inventory(inventory) {}
END_COMPONENT(Inventory)

BEGIN_COMPONENT(Dying)
    int timeToRemoval;

    Dying(int updatesTilRemoval) : timeToRemoval(updatesTilRemoval) {}
END_COMPONENT(Dying)

BEGIN_COMPONENT(Fresh)
    ECS::Signature components;

    Fresh() : components(0) {}
END_COMPONENT(Fresh)

BEGIN_COMPONENT(Inserter)
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
END_COMPONENT(Inserter)

BEGIN_COMPONENT(Rotation)
    float degrees;

    Rotation(float degrees) : degrees(degrees) {};
END_COMPONENT(Rotation)

BEGIN_COMPONENT(Rotatable)
    float start;
    float increment;
    // entity was rotated in the last update
    bool rotated;

    Rotatable(float start, float increment)
    : start(start), increment(increment) {
        rotated = false;
    }
END_COMPONENT(Rotatable)

// Entity that must have components

BEGIN_COMPONENT(Follow)
    Entity entity;
    float speed;

    Follow(Entity following, float speed)
    : entity(following), speed(speed) {
        
    }
END_COMPONENT(Follow)

BEGIN_COMPONENT(ItemStack)
    ::ItemStack item;

    ItemStack(::ItemStack itemStack) : item(itemStack) {}
END_COMPONENT(ItemStack)

BEGIN_COMPONENT(Transporter)
    IVec2 facing;
    float speed;

    Transporter(float speed) : speed(speed) {}
END_COMPONENT(Transporter)

BEGIN_COMPONENT(TransportLineEC)
    IVec2 originTile;
    float cycle; // 0 to 1
    IVec2 endTile;
    My::Vec<IVec2> belts;
END_COMPONENT(TransportLineEC)

BEGIN_COMPONENT(Immortal)
END_COMPONENT(Immortal)

BEGIN_COMPONENT(Special)
END_COMPONENT(Special)

BEGIN_COMPONENT(Dynamic)
    Dynamic(Vec2 pos) : pos(pos) {}

    Vec2 pos;
END_COMPONENT(Dynamic)

BEGIN_COMPONENT(Selected)
END_COMPONENT(Selected)

namespace Proto {
    BEGIN_PROTO_COMPONENT(Hostile)
    END_PROTO_COMPONENT(Hostile)

    FLAG_PROTO_COMPONENT(Nature)
    FLAG_PROTO_COMPONENT(Living)

    BEGIN_PROTO_COMPONENT(EntityRenderLayer)
        RenderLayer layer;
    END_PROTO_COMPONENT(EntityRenderLayer)
}

};

}

#endif