#include "world/entities/entities.hpp"
#include "ECS/ECS.hpp"
#include "world/components/components.hpp"
#include "Tiles.hpp"
#include "world/entities/methods.hpp"

namespace World {

namespace Entities {

    EC::Explosion grenadeExplosion = EC::Explosion(4, 25, 1.0f, 60);
    EC::Explosion airstrikeExplosion = EC::Explosion(8, 10000, 1.0f, 30);

    Explosive Grenade(EntityWorld* ecs, Vec2 position) {
        Explosive grenade = Explosive(ecs, position, Vec2(0.5), TextureIDs::Grenade, grenadeExplosion);
        return grenade;
    }

    Explosive Airstrike(EntityWorld* ecs, Vec2 position, Vec2 size, Vec2 target) {
        Explosive airstrike = Explosive(ecs, position, size, TextureIDs::Grenade, airstrikeExplosion);
        throwEntity(*ecs, airstrike, target, 0.2f);
        return airstrike;
    }

    Explosive ThrownGrenade(EntityWorld* ecs, Vec2 position, Vec2 target) {
        Explosive grenade = Grenade(ecs, position);
        throwEntity(*ecs, grenade, target, 0.2f);
        return grenade;
    }

    /*
    Entity Chest(EntityWorld* ecs, Vec2 position, int inventorySize, int width, int height, ItemManager& allocator) {
        Entity chest = ecs->New("chest");
        ecs->Add<EC::Position>(chest, position);
        auto min = position + Vec2{0.1, 0.1};
        auto size = Vec2{width - 0.2, height - 0.2};
        ecs->Add<EC::ViewBox>(chest, EC::ViewBox(Box{min, size}));
        ecs->Add<EC::CollisionBox>(chest, EC::CollisionBox(Box{min, size}));
        ecs->Add<EC::Render>(chest, EC::Render(TextureIDs::Chest, RenderLayers::Buildings));
        Inventory inventory = Inventory(&allocator, inventorySize);
        ecs->Add<EC::Inventory>(chest, {inventory});
        ecs->Add<EC::Growth>(chest, {0});
        return chest;
    }


    Entity Inserter(EntityWorld* ecs, Vec2 position, int reach, IVec2 inputTile, IVec2 outputTile) {
        Entity inserter = ecs->New("inserter");
        ecs->Add<EC::Position>(inserter, position);
        //TODO: ecs->Add<EC::Size>(inserter, {1.0f, 1.0f});
        ecs->Add<EC::Render>(inserter, EC::Render(TextureIDs::Inserter, RenderLayers::Buildings));
        ecs->Add<EC::Rotation>(inserter, {0.0f});
        ecs->Add<EC::Rotatable>(inserter, {0.0f, 90.0f});
        
        ecs->Add<EC::Inserter>(inserter, EC::Inserter(60, 1, reach, inputTile, outputTile));
        return inserter;
    }

    Entity Enemy(EntityWorld* ecs, Vec2 position, Entity following) {
        Entity enemy = ecs->New(PrototypeIDs::Enemy);
        ecs->StartDeferringEvents();
        ecs->Add<EC::Position>(enemy, position);
        ecs->Add<EC::Dynamic>(enemy, position);
        ecs->Add<EC::Health>(enemy, {100.0f});
        ecs->Add<EC::Rotation>(enemy, {0.0f});
        ecs->Add<EC::ViewBox>(enemy, {Box{Vec2(0), Vec2(1)}});
        ecs->Add<EC::CollisionBox>(enemy, {Box{Vec2(0.05), Vec2(0.9)}});
        ecs->Add<EC::Render>(enemy, EC::Render(TextureIDs::Player, RenderLayers::Player));
        ecs->Add<EC::Rotatable>(enemy, EC::Rotatable(0.0f, 45.0f));
        ecs->Add<EC::Follow>(enemy, EC::Follow(following, 0.05));
        ecs->StopDeferringEvents();
        return enemy;
    }
    */
}

}