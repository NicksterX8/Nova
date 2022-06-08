#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS.hpp"
#include "Components.hpp"
#include "Entity.hpp"
#include "EntityType.hpp"
#include "../Textures.hpp"

// struct Entity : public _Entity {};

namespace Entities {

    class Explosive : public EntityType<
        PositionComponent, SizeComponent, ExplosiveComponent, RenderComponent, CenteredRenderFlagComponent, RotationComponent
    >{
    public:
        Explosive(ECS* ecs, Vec2 position, Vec2 size, SDL_Texture* texture, ExplosionComponent* explosion, float startRotation = 0.0f) {
            *this = ecs->New().castType<Explosive>();

            Add<PositionComponent>(position);
            Add<SizeComponent>({size.x, size.y});
            Add<ExplosiveComponent>(ExplosiveComponent(explosion));
            Add<RenderComponent>(RenderComponent(texture, RenderLayer::Particles));
            Add<RotationComponent>({startRotation});
        }
    };

    Entity Explosion(ECS* ecs, float radius, float damage);

    void throwExplosive(ECS* ecs, Explosive explosive, Vec2 target, float speed);

    Explosive Grenade(ECS* ecs, Vec2 position, Vec2 size);
    Explosive Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target);
    Explosive ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target);
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size);

    Entity Chest(ECS* ecs, Vec2 position, int size, int width, int height);

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, RenderComponent render, MotionComponent motion);

    Entity Inserter(ECS* ecs, Vec2 position, int reach, Tile* inputTile, Tile* outputTile);

    Entity Random(ECS* ecs);

    Entity Enemy(ECS* ecs, Vec2 position, EntityType<PositionComponent> following);

    #define TEST_ENTITY_COMPONENTS {static bool test = assertType();}

    class Player : public EntityType<
        HealthComponent, NametagComponent,
        RotationComponent, SizeComponent, InventoryComponent,
        RenderComponent, PositionComponent,
        CenteredRenderFlagComponent, ImmortalEC
    >{
    public:
        Player() {}

        Player(ECS* ecs, Vec2 position) {
            *this = ecs->New().castType<Player>();
            
            Add<PositionComponent>(position);
            Add<HealthComponent>({1000});
            auto nametag = NametagComponent();
            strcpy(nametag.type, "Player");
            Add<NametagComponent>(nametag);
            Add<RotationComponent>({0.0f});
            Add<SizeComponent>({0.8f, 0.8f});
            Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
            Add<InventoryComponent>({inventory});
            Add<RenderComponent>(RenderComponent(Textures.player, RenderLayer::Player));
            Add<CenteredRenderFlagComponent>();
            Add<ImmortalEC>();

            TEST_ENTITY_COMPONENTS;
        }
    };

    class TransportBelt : public EntityType<
        HealthComponent, RotationComponent, RotatableComponent,
        RenderComponent, PositionComponent, SizeComponent
    >{
    public:
        TransportBelt(ECS* ecs, Vec2 position) {
            *this = ecs->New().castType<TransportBelt>();

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