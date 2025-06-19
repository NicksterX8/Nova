#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL3/SDL.h>
#include "utils/vectors_and_rects.hpp"
#include "items/items.hpp"
#include "ECS/Entity.hpp"
#include "ECS/componentMacros.hpp"

#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

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
        Explosive, Growth, Point, EntityTypeEC
    #define WORLD_PROTOTYPE_COMPONENT_LIST Temp
    #define WORLD_COMPONENT_LIST WORLD_REGULAR_COMPONENT_LIST COMMA WORLD_PROTOTYPE_COMPONENT_LIST
    GEN_IDS(ComponentIDs, ComponentID, WORLD_COMPONENT_LIST, Count)
};

//using SerializerOutput = void (*)(RawArray);
using SerializerOutput = const std::function<void(RawArray)>&;

template<class T>
int defaultSerializer(const T* components, Uint32 count, SerializerOutput output) {
    output(RawArray(components, sizeof(T) * count));
}

template<class T>
int defaultDeserializer(T* components, Uint32 count, const char* serialized) {
    memcpy(components, serialized, sizeof(T) * count);
}

template<class T>
using Serializer = int(const T*, Uint32, SerializerOutput);

template<class T>
using Deserializer = int(T*, Uint32, const char* serialized);

template<class T>
struct Serializable {
    static int Serialize(const T* components, Uint32 count, SerializerOutput output) {
        output(RawArray(components, sizeof(T) * count));
        return 0;
    }

    static int Deserialize(T* components, Uint32 count, const char* serialized) {
        memcpy(components, serialized, sizeof(T) * count);
        return 0;
    }
};

template<class T>
struct EntityComponent : public ECS::Component {
    private:
        static void testSerialization() {
            T::Serialize(NULL, 0, NULL);
            T::Deserialize(NULL, 0, NULL);
        }
    public:

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

/*
BEGIN_COMPONENT(Render)
    TextureID texture;
    int layer;

    Render(TextureID texture, int layer) : texture(texture), layer(layer) {}
END_COMPONENT()
*/

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

BEGIN_COMPONENT(Explosion)
    float radius;
    float damage;
    float life;
    int particleCount;

    Explosion(float radius, float damage, float life, int particleCount);
END_COMPONENT(Explosion)

BEGIN_COMPONENT(Explosive)
    Explosion explosion;

    struct SerializationData {
        char explosionName[64];
    };

    Explosive(const Explosion& explosion) : explosion(explosion) {

    }
END_COMPONENT(Explosive)

#define MAX_ENTITY_NAME_LENGTH 64

BEGIN_COMPONENT(EntityTypeEC)
    char name[MAX_ENTITY_NAME_LENGTH];
END_COMPONENT(EntityTypeEC)

BEGIN_COMPONENT(Nametag)
    char name[MAX_ENTITY_NAME_LENGTH];

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

    /*
    static int Serialize(const Inventory* components, Uint32 count, SerializerOutput output) {
        for (Uint32 i = 0; i < count; i++) {
            const ::Inventory& inventory = components[i].inventory;

            output(RawArray(inventory.size));
            output(RawArray(inventory.items, inventory.size * sizeof(::ItemStack)));
        }
        return 0;
    }

    static int Deserialize(Inventory* components, Uint32 count, const char* serialized) {
        
        
        const void* component = serialized;
        for (Uint32 i = 0; i < count; i++) {
            Uint32 size = *static_cast<const Uint32*>(component);
            const ::ItemStack* items = (const ::ItemStack*)((const char*)component + sizeof(Uint32));
            items::Inventory* inventory = &static_cast<EC::Inventory*>(components)[i].inventory;

            inventory-> = items::Inventory(size);
            memcpy(inventory->items, items, sizeof(::ItemStack) * size);
            component = (const char*)component + (sizeof(size) + (sizeof(::ItemStack) * size));

            // PROBABLY LEAKING MEMORY ON LOAD BECAUSE OF INVENTORY
        }
        return 0;
        
    }
    */
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


// proto components
BEGIN_PROTO_COMPONENT(Temp)
    int x;
END_PROTO_COMPONENT(Temp)

};

}

#endif