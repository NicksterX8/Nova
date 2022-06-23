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
    ecs->Add<EntityTypeComponent>(entity, {type});
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

    using BushType = EntityType<PositionComponent, SizeComponent, RenderComponent>;
    class Bush : public BushType {
    public:
        // using Prototype = EntityType<SizeComponent, RenderComponent>;

        void AddPrototype(ECS* ecs) {
            Add<SizeComponent>({1, 1});
            Add<RenderComponent>(RenderComponent(Textures.tree, RenderLayer::Trees));
        }

        void AddDynamic(ECS* ecs, Vec2 position) {
            Add<PositionComponent>(position);
        }

        Bush(ECS* ecs, Vec2 position) : BushType(ecs) {
            *this = createEntity(ecs, ENTITY_TYPE_BUSH).castType<Bush>();
            AddPrototype(ecs);
            AddDynamic(ecs, position);
        }
    };

    using ExplosiveType = EntityType<
        PositionComponent, SizeComponent, ExplosiveComponent, RenderComponent, RotationComponent
    >;
    class Explosive : public ExplosiveType {
    public:
        Explosive(ECS* ecs, Vec2 position, Vec2 size, SDL_Texture* texture, ExplosionComponent* explosion, float startRotation = 0.0f) : ExplosiveType(ecs) {
            *this = ecs->New().castType<Explosive>();

            Add<PositionComponent>(position);
            Add<SizeComponent>({size.x, size.y});
            Add<ExplosiveComponent>(ExplosiveComponent(explosion));
            Add<RenderComponent>(RenderComponent(texture, RenderLayer::Particles));
            Add<RotationComponent>({startRotation});
        }

        //void init(Explosive explosive) {
        //    explosive.Get<RenderComponent>()->texture = Textures.grenade;
        //}
    };

    Entity Explosion(ECS* ecs, float radius, float damage);

    void throwExplosive(ECS* ecs, Explosive explosive, Vec2 target, float speed);

    Explosive Grenade(ECS* ecs, Vec2 position, Vec2 size);
    Explosive Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target);
    Explosive ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target);
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size);

    Entity Chest(ECS* ecs, Vec2 position, int size, int width, int height);

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, RenderComponent render, MotionComponent motion);

    Entity Inserter(ECS* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile);

    Entity Random(ECS* ecs);

    Entity Enemy(ECS* ecs, Vec2 position, EntityType<PositionComponent> following);

    #define TEST_ENTITY_COMPONENTS {static bool test = assertType();}

    /*
    namespace Player {
        using Type = EntityType<
            HealthComponent, NametagComponent,
            RotationComponent, SizeComponent, InventoryComponent,
            RenderComponent, PositionComponent,
            ImmortalEC
        >;

        Type Construct(ECS* ecs, Vec2 position) {
            Type e = createEntity(ecs, "player");
            e.Add<PositionComponent>(position);
            e.Add<HealthComponent>({1000});
            e.Add<RotationComponent>({0.0f});
            e.Add<SizeComponent>({0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            e.Add<InventoryComponent>({inventory});
            e.Add<RenderComponent>({Textures.player, RenderLayer::Player});
            e.Add<ImmortalEC>();
            return e;
        }
    }*/

    using PlayerType = EntityType<
        HealthComponent, NametagComponent,
        RotationComponent, SizeComponent, InventoryComponent,
        RenderComponent, PositionComponent,
        ImmortalEC
    >;

    struct Player : public PlayerType {
        using PlayerType::PlayerType;
        Player(ECS* ecs, Vec2 position) : PlayerType(ecs) {
            Add<PositionComponent>(position);
            Add<HealthComponent>({1000});
            Add<NametagComponent>(NametagComponent("Player", ""));
            Add<RotationComponent>({0.0f});
            Add<SizeComponent>({0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            Add<InventoryComponent>({inventory});
            Add<RenderComponent>({Textures.player, RenderLayer::Player});
            Add<ImmortalEC>();

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

    DECL_ENTITY(MyEntity2, (EntityType<RenderComponent, PositionComponent>), MyEntity2Type)
        MyEntity2(ECS* ecs) : MyEntity2Type(ecs) {

        }
    };
    */

    using ItemStackType = EntityType<
        PositionComponent, SizeComponent, RenderComponent
    >;
    class ItemStack : public ItemStackType {
    public:
        ItemStack(ECS* ecs, Vec2 position, ::ItemStack item) : ItemStackType(ecs) {
            *this = ecs->New().castType<ItemStack>();

            Add<PositionComponent>(position);
            Add<SizeComponent>({0.5f, 0.5f});
            Add<ItemStackComponent>(::ItemStack(item));
            Add<GrabableEC>();
            Add<RenderComponent>(RenderComponent(ItemData[item.item].icon, RenderLayer::Items));
        }
    };

    using TransportBeltType = EntityType<
        HealthComponent, RotationComponent, RotatableComponent,
        RenderComponent, PositionComponent, SizeComponent
    >;

    class TransportBelt : public TransportBeltType {
    public:
        TransportBelt(ECS* ecs, Vec2 position) : TransportBeltType(ecs) {
            Add<HealthComponent>({100.0f});
            Add<PositionComponent>(position);
            Add<SizeComponent>({1.0f, 1.0f});
            Add<RenderComponent>(RenderComponent(Textures.inserter, RenderLayer::Buildings));
            Add<RotationComponent>({0.0f});
            Add<RotatableComponent>(RotatableComponent(0.0f, 90.0f));
            Add<TransporterEC>({0.15f});
        }
    };

    /*
    #define PlayerEntityComponents HealthComponent, NametagComponent, \
            RotationComponent, SizeComponent, InventoryComponent, \
            RenderComponent, FollowComponent, PositionComponent, \
            CenteredRenderFlagComponent
    
    using PlayerEntity = EntityType<PlayerEntityComponents>;
    PlayerEntity Player(ECS* ecs, Vec2 position);
    */

    #define ZombieEntityComponents HealthComponent, NametagComponent, \
            RotationComponent, SizeComponent, InventoryComponent, \
            RenderComponent, FollowComponent, PositionComponent, \
            FollowComponent
    
    using ZombieEntity = EntityType<ZombieEntityComponents>;
    ZombieEntity Zombie(ECS* ecs, Vec2 position, EntityType<PositionComponent> following);

}

#endif