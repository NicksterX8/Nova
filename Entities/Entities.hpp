#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "../ECS/ECS.hpp"
#include "../EntityComponents/Components.hpp"
#include "../ECS/EntityType.hpp"
#include "../Textures.hpp"

#define ENTITY_TYPE_BUSH "bush"

// struct Entity : public _Entity {};

inline Entity createEntity(ECS* ecs, const char* type) {
    Entity entity = ecs->New();
    ecs->Add<EC::EntityTypeEC>(entity, {type});
    return entity;
}

/*
Entity replicateEntityOfType(ECS* ecs, const char* type, Entity entity) {
    if (strcmp(ENTITY_TYPE_BUSH, type) == 0) {
        return Bush(ecs, entity.castType<Bush>());
    }
}
*/

namespace Entities {

    using BushType = EntityType<EC::Position, EC::Size, EC::Render>;
    class Bush : public BushType {
    public:
        // using Prototype = EntityType<EC::Size, EC::Render>;

        void AddPrototype(ECS* ecs) {
            Add<EC::Size>({1, 1});
            Add<EC::Render>(EC::Render(Textures.tree, RenderLayer::Trees));
        }

        void AddDynamic(ECS* ecs, Vec2 position) {
            Add<EC::Position>(position);
        }

        Bush(ECS* ecs, Vec2 position) : BushType(ecs) {
            *this = createEntity(ecs, ENTITY_TYPE_BUSH).castType<Bush>();
            AddPrototype(ecs);
            AddDynamic(ecs, position);
        }
    };

    using ExplosiveType = EntityType<
        EC::Position, EC::Size, EC::Explosive, EC::Render, EC::Rotation
    >;
    class Explosive : public ExplosiveType {
    public:
        Explosive(ECS* ecs, Vec2 position, Vec2 size, SDL_Texture* texture, EC::Explosion* explosion, float startRotation = 0.0f) : ExplosiveType(ecs) {
            *this = ecs->New().castType<Explosive>();

            Add<EC::Position>(position);
            Add<EC::Size>({size.x, size.y});
            Add<EC::Explosive>(EC::Explosive(explosion));
            Add<EC::Render>(EC::Render(texture, RenderLayer::Particles));
            Add<EC::Rotation>({startRotation});
        }

        //void init(Explosive explosive) {
        //    explosive.Get<EC::Render>()->texture = Textures.grenade;
        //}
    };

    Entity Explosion(ECS* ecs, float radius, float damage);

    void throwExplosive(ECS* ecs, Explosive explosive, Vec2 target, float speed);

    Explosive Grenade(ECS* ecs, Vec2 position, Vec2 size);
    Explosive Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target);
    Explosive ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target);
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size);

    Entity Chest(ECS* ecs, Vec2 position, int inventorySize, int width, int height);

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, EC::Render render, EC::Motion motion);

    Entity Inserter(ECS* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile);

    Entity Random(ECS* ecs);

    Entity Enemy(ECS* ecs, Vec2 position, EntityType<EC::Position> following);

    #define TEST_ENTITY_COMPONENTS {static bool test = assertType();}

    /*
    namespace Player {
        using Type = EntityType<
            EC::Health, EC::Nametag,
            EC::Rotation, EC::Size, EC::Inventory,
            EC::Render, EC::Position,
            EC::Immortal
        >;

        Type Construct(ECS* ecs, Vec2 position) {
            Type e = createEntity(ecs, "player");
            e.Add<EC::Position>(position);
            e.Add<EC::Health>({1000});
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Size>({0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            e.Add<EC::Inventory>({inventory});
            e.Add<EC::Render>({Textures.player, RenderLayer::Player});
            e.Add<EC::Immortal>();
            return e;
        }
    }*/

    using PlayerType = EntityType<
        EC::Health, EC::Nametag,
        EC::Rotation, EC::Size, EC::Inventory,
        EC::Render, EC::Position,
        EC::Immortal
    >;

    struct Player : public PlayerType {
        using PlayerType::PlayerType;
        Player(ECS* ecs, Vec2 position) : PlayerType(ecs) {
            Add<EC::Position>(position);
            Add<EC::Health>({1000});
            Add<EC::Nametag>(EC::Nametag("Player", ""));
            Add<EC::Rotation>({0.0f});
            Add<EC::Size>({0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            Add<EC::Inventory>({inventory});
            Add<EC::Render>({Textures.player, RenderLayer::Player});
            Add<EC::Immortal>();

            TEST_ENTITY_COMPONENTS;
        }
    };

    #define DECL_ENTITY_CTOR(name, type) name(EntityID ID, EntityVersion Version) : type(ID, Version) {}

    /*
    using MyEntityType = EntityType<A, B, C>;
    struct MyEntity : MyEntityType {
        MyEntity(EntityID ID, EntityVersion Version) : MyEntityType(ID, Version) {}
        DECL_CTOR(MyEntity, MyEntityType)

        MyEntity(ECS* ecs, ...) : MyEntityType(ecs) {

        }
    };
    */

   /*
    #define DECL_ENTITY(name, type, typename) \
        typedef type typename;\
        struct name : typename {\
        DECL_CTOR(name, typename)

    DECL_ENTITY(MyEntity2, (EntityType<EC::Render, EC::Position>), MyEntity2Type)
        MyEntity2(ECS* ecs) : MyEntity2Type(ecs) {

        }
    };
    */

    using ItemStackType = EntityType<
        EC::Position, EC::Size, EC::Render
    >;
    class ItemStack : public ItemStackType {
    public:
        ItemStack(ECS* ecs, Vec2 position, ::ItemStack item) : ItemStackType(ecs) {
            *this = ecs->New().castType<ItemStack>();

            Add<EC::Position>(position);
            Add<EC::Size>({0.5f, 0.5f});
            Add<EC::ItemStack>(::ItemStack(item));
            Add<EC::Grabable>();
            Add<EC::Render>(EC::Render(ItemData[item.item].icon, RenderLayer::Items));
        }
    };

    using TransportBeltType = EntityType<
        EC::Health, EC::Rotation, EC::Rotatable,
        EC::Render, EC::Position, EC::Size
    >;

    class TransportBelt : public TransportBeltType {
    public:
        TransportBelt(ECS* ecs, Vec2 position) : TransportBeltType(ecs) {
            Add<EC::Health>({100.0f});
            Add<EC::Position>(position);
            Add<EC::Size>({1.0f, 1.0f});
            Add<EC::Render>(EC::Render(Textures.inserter, RenderLayer::Buildings));
            Add<EC::Rotation>({0.0f});
            Add<EC::Rotatable>(EC::Rotatable(0.0f, 90.0f));
            Add<EC::Transporter>({0.15f});
        }
    };

    /*
    #define PlayerEntityComponents EC::Health, EC::Nametag, \
            EC::Rotation, EC::Size, EC::Inventory, \
            EC::Render, EC::Follow, EC::Position, \
            CenteredRenderFlagComponent
    
    using PlayerEntity = EntityType<PlayerEntityComponents>;
    PlayerEntity Player(ECS* ecs, Vec2 position);
    */

    #define ZombieEntityComponents EC::Health, EC::Nametag, \
            EC::Rotation, EC::Size, EC::Inventory, \
            EC::Render, EC::Follow, EC::Position, \
            EC::Follow
    
    using ZombieEntity = EntityType<ZombieEntityComponents>;
    ZombieEntity Zombie(ECS* ecs, Vec2 position, EntityType<EC::Position> following);

}

#endif