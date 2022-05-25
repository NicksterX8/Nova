#include "Entities.hpp"
#include "ECS.hpp"
#include "Components.hpp"

namespace Entities {

    Entity Explosive(ECS* ecs, Vec2 position, Vec2 size, SDL_Texture* texture, ExplosionComponent* explosion) {
        Entity explosive = ecs->New();
        Uint32 signature = componentSignature<
            PositionComponent,
            SizeComponent,
            ExplosiveComponent,
            RenderComponent,
            CenteredRenderFlagComponent
        >();
        ecs->Add(explosive, signature);

        ecs->Get<PositionComponent>(explosive)->x = position.x;
        ecs->Get<PositionComponent>(explosive)->y = position.y;
        ecs->Get<SizeComponent>(explosive)->width = size.x;
        ecs->Get<SizeComponent>(explosive)->height = size.y;

        *ecs->Get<ExplosiveComponent>(explosive) = ExplosiveComponent(explosion);
        
        *ecs->Get<RenderComponent>(explosive) = RenderComponent(
            texture,
            RenderLayer::Particles
        );

        return explosive;
    }

    Entity Grenade(ECS* ecs, Vec2 position, Vec2 size) {
       Entity grenade = Explosive(ecs, position, size, Textures.grenade, &grenadeExplosion);
       return grenade;
    }

    Entity Airstrike(ECS* ecs, Vec2 position, Vec2 size, Vec2 target) {
       Entity airstrike = Explosive(ecs, position, size, Textures.grenade, &airstrikeExplosion);
       throwExplosive(ecs, airstrike, target, 3.0f);
       return airstrike;
    }

    void throwExplosive(ECS* ecs, Entity explosive, Vec2 target, float speed) {
        ecs->Add<MotionComponent>(explosive, MotionComponent(target, speed));
    }

    Entity ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target) {
        Entity grenade = Grenade(ecs, position, {1, 1});
        throwExplosive(ecs, grenade, target, 0.5f);
        return grenade;
    }
    

    Entity Tree(ECS* ecs, Vec2 position, Vec2 size) {
        Entity entity = ecs->New();
        Uint32 signature = componentSignature<
            HealthComponent,
            GrowthComponent,
            PositionComponent,
            RenderComponent,
            CenteredRenderFlagComponent,
            SizeComponent,
            NametagComponent,
            InventoryComponent
        >();

        ecs->Add(entity, signature);

        ecs->Get<HealthComponent>(entity)->healthValue = 100;
        *ecs->Get<PositionComponent>(entity) = PositionComponent(position);
        ecs->Get<GrowthComponent>(entity)->growthValue = 0;
        *ecs->Get<RenderComponent>(entity) = RenderComponent(Textures.tree, RenderLayer::Trees);
        ecs->Get<SizeComponent>(entity)->width = size.x;
        ecs->Get<SizeComponent>(entity)->height = size.y;
        Inventory inventory = Inventory(10);
        ecs->Get<InventoryComponent>(entity)->inventory = inventory;

        strcpy(ecs->Get<NametagComponent>(entity)->type, "tree");
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

    Entity Random(ECS* ecs) {
        Entity entity = ecs->New();
        Uint32 signature = componentSignature<
            COMPONENTS
        >();
        ecs->Add(entity, signature);

        return entity;
    }

}