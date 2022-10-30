#include "ECS/entities/entities.hpp"
#include "ECS/ECS.hpp"
#include "ECS/components/components.hpp"
#include "Tiles.hpp"
#include "ECS/SEntity.hpp"
#include "ECS/entities/methods.hpp"

namespace Entities {

    EC::Explosion grenadeExplosion = EC::Explosion(4, 25, 1.0f, 60);
    EC::Explosion airstrikeExplosion = EC::Explosion(8, 10000, 1.0f, 30);

    Explosive Grenade(EntityWorld* ecs, Vec2 position, Vec2 size) {
        Explosive grenade = Explosive(ecs, position, size, TextureIDs::Grenade, grenadeExplosion);
        return grenade;
    }

    Explosive Airstrike(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target) {
        Explosive airstrike = Explosive(ecs, position, size, TextureIDs::Grenade, airstrikeExplosion);
        throwEntity(ecs, airstrike, target, 0.2f);
        return airstrike;
    }

    Explosive ThrownGrenade(EntityWorld* ecs, Vec2 position, Vec2 target) {
        Explosive grenade = Grenade(ecs, position, {1, 1});
        throwEntity(ecs, grenade, target, 0.2f);
        return grenade;
    }

    Entity Chest(EntityWorld* ecs, Vec2 position, int inventorySize, int width, int height) {
        Entity chest = ecs->New("chest");
        ecs->Add<EC::Position>(chest, position);
        ecs->Add<EC::Size>(chest, {(float)width - 0.2f, (float)height - 0.2f});
        ecs->Add<EC::Render>(chest, EC::Render(TextureIDs::Chest, RenderLayer::Buildings));
        Inventory inventory = Inventory(inventorySize);
        ecs->Add<EC::Inventory>(chest, {inventory});
        ecs->Add<EC::Growth>(chest, {0});
        return chest;
    }

    Entity Particle(EntityWorld* ecs, Vec2 position, Vec2 size, EC::Render render, EC::Motion motion) {
        Entity particle = ecs->New("particle");
        ecs->Add<EC::Position>(particle, position);
        ecs->Add<EC::Size>(particle, {size.x, size.y});
        ecs->Add<EC::Motion>(particle, motion);
        ecs->Add<EC::Render>(particle, render);
        float distance = (motion.target - position).length();
        ecs->Add<EC::Dying>(particle, (EC::Dying){(int)ceil(distance / motion.speed)});
        return particle;
    }

    Entity Inserter(EntityWorld* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile) {
        Entity inserter = ecs->New("inserter");
        ecs->Add<EC::Position>(inserter, position);
        ecs->Add<EC::Size>(inserter, {1.0f, 1.0f});
        ecs->Add<EC::Render>(inserter, EC::Render(TextureIDs::Inserter, RenderLayer::Buildings));
        ecs->Add<EC::Rotation>(inserter, {0.0f});
        ecs->Add<EC::Rotatable>(inserter, {0.0f, 90.0f});
        
        ecs->Add<EC::Inserter>(inserter, EC::Inserter(60, 1, reach, inputTile, outputTile));
        return inserter;
    }

    Entity Enemy(EntityWorld* ecs, Vec2 position, EntityT<EC::Position> following) {
        Entity enemy = ecs->New("enemy");
        ecs->Add<EC::Position>(enemy, position);
        ecs->Add<EC::Health>(enemy, {100.0f});
        ecs->Add<EC::Nametag>(enemy, EC::Nametag("Enemy", ""));
        ecs->Add<EC::Rotation>(enemy, {0.0f});
        ecs->Add<EC::Size>(enemy, {0.8f, 0.8f});
        ecs->Add<EC::Render>(enemy, EC::Render(TextureIDs::Player, RenderLayer::Player));
        ecs->Add<EC::Rotatable>(enemy, EC::Rotatable(0.0f, 45.0f));
        ecs->Add<EC::Follow>(enemy, EC::Follow(following, 0.05));
        return enemy;
    }
}