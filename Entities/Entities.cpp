#include "Entities.hpp"
#include "../ECS/ECS.hpp"
#include "../EntityComponents/Components.hpp"
#include "../Tiles.hpp"
#include "../ECS/EntityType.hpp"

namespace Entities {

    EC::Explosion grenadeExplosion = EC::Explosion(4, 25, 1.0f, 60);
    EC::Explosion airstrikeExplosion = EC::Explosion(8, 10000, 1.0f, 30);

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
        explosive.Add<EC::Motion>(EC::Motion(target, speed));
        if (explosive.Has<EC::Rotation>()) {
            float rotation = explosive.Get<EC::Rotation>()->degrees;
            float timeToTarget = target.length() / speed;
            explosive.Add<EC::AngleMotion>(EC::AngleMotion(rotation + timeToTarget * 30.0f, 30.0f));
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
            EC::Health,
            EC::Growth,
            EC::Position,
            EC::Render,
            EC::Size,
            EC::Nametag,
            EC::Inventory
        >();

        ecs->AddSignature(entity, signature);

        ecs->Get<EC::Health>(entity)->healthValue = 100;
        *ecs->Get<EC::Position>(entity) = EC::Position(position);
        ecs->Get<EC::Growth>(entity)->growthValue = 0;
        *ecs->Get<EC::Render>(entity) = EC::Render(Textures.tree, RenderLayer::Trees);
        ecs->Get<EC::Size>(entity)->width = size.x;
        ecs->Get<EC::Size>(entity)->height = size.y;
        Inventory inventory = Inventory(10);
        ecs->Get<EC::Inventory>(entity)->inventory = inventory;
        ecs->Get<EC::Nametag>(entity)->setType("Tree");
        return entity;
    }

    Entity Chest(ECS* ecs, Vec2 position, int inventorySize, int width, int height) {
        Entity chest = ecs->New();
        ecs->Add<EC::Position>(chest, position);
        ecs->Add<EC::Size>(chest, {(float)width - 0.2f, (float)height - 0.2f});
        ecs->Add<EC::Render>(chest, EC::Render(Textures.chest, RenderLayer::Buildings));
        Inventory inventory = Inventory(inventorySize);
        ecs->Add<EC::Inventory>(chest, {inventory});
        ecs->Add<EC::Growth>(chest, {0});
        return chest;
    }

    Entity Particle(ECS* ecs, Vec2 position, Vec2 size, EC::Render render, EC::Motion motion) {
        Entity particle = ecs->New();
        ecs->Add<EC::Position>(particle, position);
        ecs->Add<EC::Size>(particle, {size.x, size.y});
        ecs->Add<EC::Motion>(particle, motion);
        ecs->Add<EC::Render>(particle, render);
        float distance = (motion.target - position).length();
        ecs->Add<EC::Dying>(particle, (EC::Dying){(int)ceil(distance / motion.speed)});
        return particle;
    }

    Entity Inserter(ECS* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile) {
        Entity inserter = ecs->New();
        ecs->Add<EC::Position>(inserter, position);
        ecs->Add<EC::Size>(inserter, {1.0f, 1.0f});
        ecs->Add<EC::Render>(inserter, EC::Render(Textures.inserter, RenderLayer::Buildings));
        ecs->Add<EC::Rotation>(inserter, {0.0f});
        ecs->Add<EC::Rotatable>(inserter, {0.0f, 90.0f});
        
        ecs->Add<EC::Inserter>(inserter, EC::Inserter(60, 1, reach, inputTile, outputTile));
        return inserter;
    }

    Entity Random(ECS* ecs) {
        Entity entity = ecs->New();
        ComponentFlags signature = componentSignature<
            COMPONENTS
        >();
        ecs->AddSignature(entity, signature);

        return entity;
    }

    Entity Enemy(ECS* ecs, Vec2 position, EntityType<EC::Position> following) {
        Entity enemy = ecs->New();
        ecs->Add<EC::Position>(enemy, position);
        ecs->Add<EC::Health>(enemy, {100.0f});
        ecs->Add<EC::Nametag>(enemy, EC::Nametag("Enemy", ""));
        ecs->Add<EC::Rotation>(enemy, {0.0f});
        ecs->Add<EC::Size>(enemy, {0.8f, 0.8f});
        ecs->Add<EC::Render>(enemy, EC::Render(Textures.player, RenderLayer::Player));
        ecs->Add<EC::Rotatable>(enemy, EC::Rotatable(0.0f, 45.0f));
        ecs->Add<EC::Follow>(enemy, EC::Follow(following, 0.05));
        return enemy;
    }

    /*
    PlayerEntity Player(ECS* ecs, Vec2 position) {
        EntityType<> player = ecs->New().cast<>();
        ecs->Add<EC::Position>(player, position);
        ecs->Add<EC::Health>(player, {INFINITY});
        auto nametag = EC::Nametag();
        strcpy(nametag.type, "Player");
        ecs->Add<EC::Nametag>(player, nametag);
        ecs->Add<EC::Rotation>(player, {0.0f});
        ecs->Add<EC::Size>(player, {0.8f, 0.8f});
        Inventory inventory = Inventory(PLAYER_INVENTORY_SIZE);
        ecs->Add<EC::Inventory>(player, {inventory});
        ecs->Add<EC::Render>(player, EC::Render(Textures.player, RenderLayer::Player));
        ecs->Add<CenteredRenderFlagComponent>(player);
        ecs->Add<EC::Follow>(player, EC::Follow(player.cast<EC::Position>(), 1.0f));

        Zombie(ecs, position);
        Skeleton skelly = Skeleton();

        Entity hidden = Skeleton();
        skelly = hidden.castType<Skeleton>();

        player.Get<EC::Position>()->x = 100;

        static bool test = assertEntityType<PlayerEntityComponents>(player, ecs);

        return assertCastEntityType<PlayerEntityComponents>(player, ecs);
    }
    */

    ZombieEntity Zombie(ECS* ecs, Vec2 position, EntityType<EC::Position> following) {
        SafeEntity<> ent = ecs->New().cast<>();

        auto em = ecs->em();

        auto result = (ent
            .Add<EC::Size>({1, 1})
            .Add<EC::Position>()
            .Add<EC::Render>()
            .Add<EC::Nametag>()
            .Add<EC::Inventory>()
            .Add<EC::Health>()
            .Add<EC::Rotation>()
            .Add<EC::Follow>(EC::Follow(following, 0.05))
        );
    
        return result;
    }
}