#include "Entities.hpp"
#include "ECS.hpp"
#include "Components.hpp"

namespace Entities {

    Entity Grenade(ECS* ecs, Vec2 position, Vec2 size) {
        Entity grenade = ecs->New();
        Uint32 signature = componentSignature<
            PositionComponent,
            SizeComponent,
            ExplosiveComponent,
            RenderComponent,
            CenteredRenderFlagComponent,
            NametagComponent
        >();
        ecs->Add(grenade, signature);

        strcpy(ecs->Get<NametagComponent>(grenade)->type, "grenade");

        ecs->Get<PositionComponent>(grenade)->x = position.x;
        ecs->Get<PositionComponent>(grenade)->y = position.y;
        ecs->Get<SizeComponent>(grenade)->width = size.x;
        ecs->Get<SizeComponent>(grenade)->height = size.y;

        *ecs->Get<ExplosiveComponent>(grenade) = ExplosiveComponent(&grenadeExplosion);
        
        *ecs->Get<RenderComponent>(grenade) = RenderComponent(
            Textures.grenade,
            RenderLayer::Particles
        );

        return grenade;
    }

    Entity ThrownGrenade(ECS* ecs, Vec2 position, Vec2 target) {
        Entity grenade = Grenade(ecs, position, {1, 1});
        ecs->Add<MotionComponent>(grenade, MotionComponent(target, 0.5f));
        
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
            NametagComponent
        >();

        ecs->Add(entity, signature);

        ecs->Get<HealthComponent>(entity)->healthValue = 100;
        *ecs->Get<PositionComponent>(entity) = PositionComponent(position);
        ecs->Get<GrowthComponent>(entity)->growthValue = 0;
        *ecs->Get<RenderComponent>(entity) = RenderComponent(Textures.tree, RenderLayer::Trees);
        ecs->Get<SizeComponent>(entity)->width = size.x;
        ecs->Get<SizeComponent>(entity)->height = size.y;

        strcpy(ecs->Get<NametagComponent>(entity)->type, "tree");
        return entity;
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