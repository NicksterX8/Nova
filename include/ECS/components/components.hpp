#ifndef COMPONENTS_INCLUDED
#define COMPONENTS_INCLUDED

#include <SDL3/SDL.h>
#include "utils/vectors_and_rects.hpp"
#include "items/items.hpp"
#include "ECS/SEntity.hpp"

struct RawArray {
    const void* data;
    const size_t size;

    RawArray(const void* ptr, size_t size) : data(ptr), size(size) {}

    template<class T>
    RawArray(const T& value) : data(&value), size(sizeof(value)) {}
};

struct EntityWorld;

namespace EC {

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

        void construct(EntityWorld*, Entity) {}
        void deconstruct(EntityWorld*, Entity) {}
};

#define ENTITY_TYPE_NAME_SIZE 64

struct EntityTypeEC : EntityComponent<EntityTypeEC> {
    //Uint32 id;
    char name[ENTITY_TYPE_NAME_SIZE];

    EntityTypeEC(const char* _name) {
        strcpy(this->name, _name);
    }

    static int Serialize(const EntityTypeEC* components, Uint32 count, SerializerOutput output) {
        output(RawArray(components, count * sizeof(EntityTypeEC)));
        return 0;
    }

    static int Deserialize(EntityTypeEC* components, Uint32 count, const char* serialized) {
        memcpy(components, serialized, sizeof(EntityTypeEC) * count);
        return 0;
    }
};

struct Grabable : EntityComponent<Grabable>, Serializable<Grabable> {
    ::ItemStack itemGiven;

    Grabable(::ItemStack itemGiven) : itemGiven(itemGiven) {}
};

struct Health : EntityComponent<Health>, Serializable<Health> {
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
};

struct Growth : EntityComponent<Growth>, Serializable<Growth> {
    float growth;

    Growth(float Growth) : growth(Growth) {}
};

struct ViewBox : EntityComponent<ViewBox>, Serializable<ViewBox> {
    Box box;

    constexpr ViewBox(Box box) : box(box) {}

    constexpr ViewBox(Vec2 min, Vec2 size) : box(Box{min, size}) {}

    static constexpr ViewBox BottomLeft(Vec2 size) {
        return ViewBox(Vec2(0.0, 0.0), size);
    }

    static constexpr ViewBox Centered(Vec2 size) {
        return ViewBox{Box{size/2.0f, size}};
    }
        
};

struct CollisionBox : EntityComponent<CollisionBox>, Serializable<CollisionBox> {
    Box box;

    CollisionBox(Box box) : box(box) {}

    CollisionBox(Vec2 min, Vec2 size) {
        box = Box{min, size};
    }
};

struct Point : EntityComponent<Point>, Serializable<Point> {
    Vec2 p;

    Point(Vec2 p) : p(p) {}
};

struct Position : EntityComponent<Position>, Serializable<Position> {
    float x;
    float y;

    Position(float x, float y) : x(x), y(y) {}
    Position(Vec2 vec) : x(vec.x), y(vec.y) {}
    
    Vec2 vec2() const {
        return Vec2(x, y);
    }
};

struct Size : EntityComponent<Size>, Serializable<Size> {
    float width;
    float height;

    Size(float width, float height) : width(width), height(height) {}
    Size(Vec2 vec) : width(vec.x), height(vec.y) {}

    Vec2 vec2() const {
        return Vec2(width, height);
    };
};

/*
struct Render : EntityComponent<Render>, Serializable<Render> {
    TextureID texture;
    int layer;

    Render(TextureID texture, int layer) : texture(texture), layer(layer) {}
};
*/

#define RENDER_COMPONENT_MAX_TEX 3
struct Render : EntityComponent<Render> {
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
};

struct Explosion : EntityComponent<Explosion>, Serializable<Explosion> {
    float radius;
    float damage;
    float life;
    int particleCount;

    Explosion(float radius, float damage, float life, int particleCount);
};

struct ExplosionComponentType {
    char name[64];
    Explosion explosion;
};

struct Explosive : EntityComponent<Explosive>, Serializable<Explosive> {
    Explosion explosion;

    struct SerializationData {
        char explosionName[64];
    };

    Explosive(const Explosion& explosion) : explosion(explosion) {

    }
};

#define MAX_ENTITY_NAME_LENGTH 64

struct Nametag : EntityComponent<Nametag>, Serializable<Nametag>{
    char name[MAX_ENTITY_NAME_LENGTH];

    void setName(const char* name);

    Nametag() {
        name[0] = '\0';
    }
    Nametag(const char* name) {
        setName(name);
    }
};

struct Motion : EntityComponent<Motion>, Serializable<Motion> {
    Vec2 target;
    float speed;

    Motion(Vec2 target, float speed) : target(target), speed(speed) {}
};

struct AngleMotion : EntityComponent<AngleMotion>, Serializable<AngleMotion> {
    float rotationTarget;
    float rotationSpeed;

    AngleMotion(float rotationTarget, float rotationSpeed) : rotationTarget(rotationTarget), rotationSpeed(rotationSpeed) {}
};

struct Inventory : EntityComponent<Inventory> {
    ::Inventory inventory;

    Inventory(::Inventory inventory) : inventory(inventory) {}

    void construct(EntityWorld* ecs, Entity entity) {
        inventory.addRef();
    }

    void deconstruct(EntityWorld* ecs, Entity entity) {
        inventory.removeRef();
    }

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
};

struct Dying : EntityComponent<Dying>, Serializable<Dying> {
    int timeToRemoval;

    Dying(int updatesTilRemoval) : timeToRemoval(updatesTilRemoval) {}
};

struct Fresh : EntityComponent<Fresh>, Serializable<Fresh> {
    ComponentFlags flags;

    Fresh() : flags(0) {}
};

struct Inserter : EntityComponent<Inserter>, Serializable<Inserter> {
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

struct Rotation : EntityComponent<Rotation>, Serializable<Rotation> {
    float degrees;

    Rotation(float degrees) : degrees(degrees) {};
};

struct Rotatable : EntityComponent<Rotatable>, Serializable<Rotatable> {
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

struct Follow : EntityComponent<Follow>, Serializable<Follow> {
    Entity entity;
    float speed;

    Follow(Entity following, float speed)
    : entity(following), speed(speed) {
        
    }
};

struct ItemStack : EntityComponent<ItemStack>, Serializable<ItemStack> {
    ::ItemStack item;

    ItemStack(::ItemStack itemStack) : item(itemStack) {}
};

struct Transporter : EntityComponent<Transporter>, Serializable<Transporter> {
    IVec2 facing;
    float speed;

    Transporter(float speed) : speed(speed) {}
};

struct TransportLineEC : EntityComponent<TransportLineEC>, Serializable<TransportLineEC> {
    IVec2 originTile;
    float cycle; // 0 to 1
    IVec2 endTile;
    My::Vec<IVec2> belts;
};

struct Immortal : EntityComponent<Immortal>, Serializable<Immortal> {};

struct Special : EntityComponent<Special>, Serializable<Special> {};

struct Dynamic : EntityComponent<Dynamic>, Serializable<Dynamic> {
    Dynamic(Vec2 pos) : pos(pos) {}

    Vec2 pos;
};

struct Selected : EntityComponent<Selected>, Serializable<Selected> {};

}

#endif