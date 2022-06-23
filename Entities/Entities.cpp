#include "Entities.hpp"
#include "../ECS/ECS.hpp"
#include "../EntityComponents/Components.hpp"
#include "../Tiles.hpp"
#include "../ECS/EntityType.hpp"

namespace Entities {

    ExplosionComponent grenadeExplosion = ExplosionComponent(4, 25, 1.0f, 60);
    ExplosionComponent airstrikeExplosion = ExplosionComponent(8, 10000, 1.0f, 30);

    Explosive Grenade(ECS* ecs, Vec2 position, Vec2 size) {
       Explosive grenade = Explosive(ecs, position, size, Textures.grenade, &grenadeExplosion);
       return grenade;
    }

    Explosive Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target) {
       Explosive airstrike = Explosive(ecs, position, size, Textures.grenade, &airstrikeExplosion);
       throwExplosive(ecs, airstrike, target, 2.0f);
       return airstrike;
    }

    void throwExplosive(ECS* ecs, Explosive explosive, Vec2 target, float speed) {
        explosive.Add<MotionComponent>(MotionComponent(target, speed));
        if (explosive.Has<RotationComponent>()) {
            float rotation = explosive.Get<RotationComponent>()->degrees;
            float timeToTarget = target.length() / speed;
            explosive.Add<AngleMotionEC>(AngleMotionEC(rotation + timeToTarget * 30.0f, 30.0f));
        }
    }

    Explosive ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target) {
        Explosive grenade = Grenade(ecs, position, {1, 1});
        throwExplosive(ecs, grenade, target, 0.2f);
        return grenade;
    }
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size) {
        Entity entity = ecs->New();
        ComponentFlags signature = componentSignature<
            HealthComponent,
            GrowthComponent,
            PositionComponent,
            RenderComponent,
            SizeComponent,
            NametagComponent,
            InventoryComponent
        >();

        ecs->AddSignature<COMPONENTS>(entity, signature);

        ecs->Get<HealthComponent>(entity)->healthValue = 100;
        *ecs->Get<PositionComponent>(entity) = PositionComponent(position);
        ecs->Get<GrowthComponent>(entity)->growthValue = 0;
        *ecs->Get<RenderComponent>(entity) = RenderComponent(Textures.tree, RenderLayer::Trees);
        ecs->Get<SizeComponent>(entity)->width = size.x;
        ecs->Get<SizeComponent>(entity)->height = size.y;
        Inventory inventory = Inventory(10);
        ecs->Get<InventoryComponent>(entity)->inventory = inventory;
        ecs->Get<NametagComponent>(entity)->setType("Tree");
        return entity;
    }

    Entity Chest(ECS* ecs, Vec2 position, int size, int width, int height) {
        Entity chest = ecs->New();
        ecs->Add<PositionComponent>(chest, (Vec2)(position + Vec2(0.1, 0.1)));
        ecs->Add<SizeComponent>(chest, {(float)width - 0.2f, (float)height - 0.2f});
        ecs->Add<RenderComponent>(chest, RenderComponent(Textures.chest, RenderLayer::Buildings));
        Inventory inventory = Inventory(64);
        ecs->Add<InventoryComponent>(chest, {inventory});
        ecs->Add<GrowthComponent>(chest, {0});
        return chest;
    }

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, RenderComponent render, MotionComponent motion) {
        Entity particle = ecs->New();
        ecs->Add<PositionComponent>(particle, position);
        ecs->Add<SizeComponent>(particle, {size.x, size.y});
        ecs->Add<MotionComponent>(particle, motion);
        ecs->Add<RenderComponent>(particle, render);
        float distance = (motion.target - position).length();
        ecs->Add<DyingComponent>(particle, (DyingComponent){(int)ceil(distance / motion.speed)});
        return particle;
    }

    Entity Inserter(ECS* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile) {
        Entity inserter = ecs->New();
        ecs->Add<PositionComponent>(inserter, position);
        ecs->Add<SizeComponent>(inserter, {1.0f, 1.0f});
        ecs->Add<RenderComponent>(inserter, RenderComponent(Textures.inserter, RenderLayer::Buildings));
        ecs->Add<RotationComponent>(inserter, {0.0f});
        ecs->Add<RotatableComponent>(inserter, {0.0f, 90.0f});
        
        ecs->Add<InserterComponent>(inserter, InserterComponent(60, 1, reach, inputTile, outputTile));
        return inserter;
    }

    Entity Random(ECS* ecs) {
        Entity entity = ecs->New();
        ComponentFlags signature = componentSignature<
            COMPONENTS
        >();
        ecs->AddSignature<COMPONENTS>(entity, signature);

        return entity;
    }

    Entity Enemy(ECS* ecs, Vec2 position, EntityType<PositionComponent> following) {
        Entity enemy = ecs->New();
        ecs->Add<PositionComponent>(enemy, position);
        ecs->Add<HealthComponent>(enemy, {100.0f});
        ecs->Add<NametagComponent>(enemy, NametagComponent("Enemy", ""));
        ecs->Add<RotationComponent>(enemy, {0.0f});
        ecs->Add<SizeComponent>(enemy, {0.8f, 0.8f});
        ecs->Add<RenderComponent>(enemy, RenderComponent(Textures.player, RenderLayer::Player));
        ecs->Add<RotatableComponent>(enemy, RotatableComponent(0.0f, 45.0f));
        ecs->Add<FollowComponent>(enemy, FollowComponent(following, 0.05));
        return enemy;
    }

    /*
    PlayerEntity Player(ECS* ecs, Vec2 position) {
        EntityType<> player = ecs->New().cast<>();
        ecs->Add<PositionComponent>(player, position);
        ecs->Add<HealthComponent>(player, {INFINITY});
        auto nametag = NametagComponent();
        strcpy(nametag.type, "Player");
        ecs->Add<NametagComponent>(player, nametag);
        ecs->Add<RotationComponent>(player, {0.0f});
        ecs->Add<SizeComponent>(player, {0.8f, 0.8f});
        Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
        ecs->Add<InventoryComponent>(player, {inventory});
        ecs->Add<RenderComponent>(player, RenderComponent(Textures.player, RenderLayer::Player));
        ecs->Add<CenteredRenderFlagComponent>(player);
        ecs->Add<FollowComponent>(player, FollowComponent(player.cast<PositionComponent>(), 1.0f));

        Zombie(ecs, position);
        Skeleton skelly = Skeleton();

        Entity hidden = Skeleton();
        skelly = hidden.castType<Skeleton>();

        player.Get<PositionComponent>()->x = 100;

        static bool test = assertEntityType<PlayerEntityComponents>(player, ecs);

        return assertCastEntityType<PlayerEntityComponents>(player, ecs);
    }
    */

    ZombieEntity Zombie(ECS* ecs, Vec2 position, EntityType<PositionComponent> following) {
        SafeEntity<> ent = ecs->New().cast<>();

        auto em = ecs->em();

        auto result = (ent
            .Add<SizeComponent>({1, 1})
            .Add<PositionComponent>()
            .Add<RenderComponent>()
            .Add<NametagComponent>()
            .Add<InventoryComponent>()
            .Add<HealthComponent>()
            .Add<RotationComponent>()
            .Add<FollowComponent>(FollowComponent(following, 0.05))
        );
    
        return result;
    }
}