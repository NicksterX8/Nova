#ifndef SYSTEMS_INCLUDED
#define SYSTEMS_INCLUDED

#include <vector>
#include <random>
#include "../ECS.hpp"
#include "../EntitySystem.hpp"
#include "../EntityManager.hpp"
#include "../EntityType.hpp"

#include "../../Log.hpp"
#include "../../constants.hpp"

static std::default_random_engine randomGen;

class RotationSystem : public EntitySystem {
public:
    RotationSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<RotationComponent>()]);
    }

    void Update(ECS* ecs) {
        setEntitiesRenderRotation(ecs);
    }

private:

    void setEntitiesRenderRotation(ECS* ecs) {
        ForEach<RotationComponent, RenderComponent>(ecs, [&](auto entity){
            // copy rotation to render component
            #define GET(entity, component) ecs->Get<component>(entity)
            ecs->Get<RenderComponent>(entity)->rotation = ecs->Get<RotationComponent>(entity)->degrees;
            // entity.template Add<PositionComponent>(ecs, {1, 1});
            // ecs->Add<PositionComponent>(entity, {1, 1});
        });

        ForEach<AngleMotionEC, RotationComponent>(ecs, [&](EntityType<AngleMotionEC, RotationComponent> entity){
            float* rotation = &entity.Get<RotationComponent>()->degrees;
            auto angleMotion = entity.Get<AngleMotionEC>();
            if (fabs(*rotation - angleMotion->rotationTarget) < angleMotion->rotationSpeed) {
                // reached target
                *rotation = angleMotion->rotationTarget;
                ecs->ScheduleRemoveComponents<AngleMotionEC>(entity);
                return;
            }

            *rotation += angleMotion->rotationSpeed;
        });
    }
};

class RotatableSystem : public EntitySystem {
public:
    RotatableSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<RotatableComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach([&](auto entity){
            ecs->Get<RotatableComponent>(entity)->rotated = false;
        });
    }
};

class GrowthSystem : public EntitySystem {
public:
    GrowthSystem() : EntitySystem(1000) {

    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<GrowthComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach([&](auto entity){
            ecs->Get<GrowthComponent>(entity)->growthValue += 1;
        });
    }
private:

};

class MotionSystem : public EntitySystem {
public:
    MotionSystem() : EntitySystem() {

    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<MotionComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach<PositionComponent, MotionComponent>(ecs, [&](auto entity){
            PositionComponent* position = ecs->Get<PositionComponent>(entity);
            Vec2 target = ecs->Get<MotionComponent>(entity)->target;
            Vec2 delta = {target.x - position->x, target.y - position->y};
            float speed = ecs->Get<MotionComponent>(entity)->speed;
            Vec2 unit = delta.norm().scaled(speed);

            if (delta.length() < speed) {
                position->x = target.x;
                position->y = target.y;

                ecs->ScheduleRemoveComponents<MotionComponent>(entity);
            } else {
                position->x += unit.x;
                position->y += unit.y;
            }
        });
    }
private:

};

class FollowSystem : public EntitySystem {
public:
    FollowSystem() : EntitySystem() {}

    bool Query(ComponentFlags signature) const {
        return (signature[getID<FollowComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach<PositionComponent, FollowComponent>(ecs, [&](EntityType<PositionComponent, FollowComponent> entity){
            auto followComponent = entity.Get<FollowComponent>();
            if (!ecs->EntityLives(followComponent->entity)) {
                return;
            }
            EntityType<PositionComponent> following = followComponent->entity;
            Vec2 target = *following.Get<PositionComponent>();

            PositionComponent* position = ecs->Get<PositionComponent>(entity);
            Vec2 delta = {target.x - position->x, target.y - position->y};
            Vec2 unit = delta.norm().scaled(followComponent->speed);

            if (delta.length() < followComponent->speed) {
                position->x = target.x;
                position->y = target.y;

                // do something
                // hurt them if they have health
                if (following.Has<HealthComponent>()) {
                    following.Get<HealthComponent>()->healthValue -= 10;
                }

            } else {
                position->x += unit.x;
                position->y += unit.y;
            }
        });
    }
};

class HealthSystem : public EntitySystem {
public:
    HealthSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<HealthComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach([&](Entity entity){
            float* health = &ecs->Get<HealthComponent>(entity)->healthValue;
            // Must do check like this instead of (*health <= 0.0f) to account for NaN values,
            // which can occur when infinite damage is done to an entity with infinite health
            // so in that situation the infinite damage wins out, rather than the infinte health
            if (!(*health > 0.0f)) {
                if (!ecs->entityComponents(entity.id)[getID<ImmortalEC>()])
                    ecs->ScheduleRemove(entity);
            }
        });
    }
};

class InventorySystem : public EntitySystem {
public:
    InventorySystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<InventoryComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach<GrowthComponent>(ecs, [&](auto entity){
            GrowthComponent* growth = ecs->Get<GrowthComponent>(entity);
            if (growth->growthValue > 100) {
                growth->growthValue -= 100;
                Inventory* inventory = &ecs->Get<InventoryComponent>(entity)->inventory;
                inventory->addItemStack(ItemStack(Items::Wall));
            }
        });
    }
};

class ExplosivesSystem : public EntitySystem {
public:
    ExplosivesSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<ExplosiveComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach<PositionComponent, MotionComponent>(ecs, [&](auto entity){
            Vec2 target = ecs->Get<MotionComponent>(entity)->target;
            Vec2 position = *ecs->Get<PositionComponent>(entity);

            if (target.x == position.x && target.y == position.y) {
                // EXPLODE
                
                Entity explosion = ecs->New();
                auto explosionComponent = *ecs->Get<ExplosiveComponent>(entity)->explosion;
                ecs->Add<ExplosionComponent>(explosion, explosionComponent);
                ecs->Add<PositionComponent>(explosion, PositionComponent(target));

                float radius = explosionComponent.radius;

                std::uniform_int_distribution<float> speedDist(0.2f, 1.0f);
                std::uniform_int_distribution<float> angleDist(-10*M_PI, 10*M_PI);
                std::uniform_int_distribution<float> distDist(-radius, radius);
                std::uniform_int_distribution<float> sizeDist(0.6f, 1.2f);

                // spawn particles
                for (int i = 0; i < explosionComponent.particleCount; i++) {
                    
                    float speed = speedDist(randomGen);
                    float angle = angleDist(randomGen);
                    float dist = distDist(randomGen); 
                    float size = sizeDist(randomGen);

                    Vec2 particleTarget = {
                        target.x + dist * cos(angle),
                        target.y + dist * sin(angle)
                    };

                    Entity particle = Entities::Particle(
                        ecs,
                        target,
                        {size, size},
                        RenderComponent(Textures.Tiles.sand, RenderLayer::Particles),
                        MotionComponent(particleTarget, speed)
                    );

                    ecs->Add<RotationComponent>(particle, {angle});
                    ecs->Add<AngleMotionEC>(particle, AngleMotionEC(angle + dist * 30, speed));
                }

                ecs->ScheduleRemove(entity);
                
            }
        });
    }

};

class ExplosionSystem : public EntitySystem {
public:
    ExplosionSystem() : EntitySystem() {}

    bool Query(ComponentFlags signature) const {
        return (signature[getID<ExplosionComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach<PositionComponent, ExplosionComponent>(ecs, [&](auto entity){
            ExplosionComponent* explosion = ecs->Get<ExplosionComponent>(entity);
            float radius = explosion->radius;
            //PositionComponent* positionComponent = ecs->Get<PositionComponent>(entity);
            //Vec2 position = {positionComponent->x, positionComponent->y};
            Vec2 position = *ecs->Get<PositionComponent>(entity);
            
            // search for entities to kill
            ecs->em()->iterateEntities<PositionComponent, HealthComponent>([&](Entity affectedEntity){
                if (entity == affectedEntity) {
                    // explosion component shouldn't affect itself
                    return;
                }

                Vec2 aPos = *ecs->Get<PositionComponent>(affectedEntity);
                if (ecs->entityComponents(affectedEntity.id)[getID<SizeComponent>()]) {
                    auto size = ecs->Get<SizeComponent>(affectedEntity);
                    aPos.x += size->width/2;
                    aPos.y += size->height/2;
                }
                Vec2 delta = aPos - position;
                float distanceSqrd = delta.x * delta.x + delta.y * delta.y;
                
                if (distanceSqrd < radius*radius) {
                    // entity is in range of explosion,
                    // reduce their health
                    ecs->Get<HealthComponent>(affectedEntity)->healthValue -= explosion->damage;
                }
            });
            
            explosion->life--;
            if (explosion->life < 1) {
                ecs->ScheduleRemove(entity);
            }
        });
    }
};

class InserterSystem : public EntitySystem {
public:
    InserterSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        ComponentFlags need = componentSignature<
            InserterComponent, PositionComponent, RotationComponent, RotatableComponent
        >();
        return ((entitySignature & need) == need);
    }

    void Update(ECS* ecs, ChunkMap& chunkmap) {
        ForEach([&](Entity entity){
            auto inserter = ecs->Get<InserterComponent>(entity);
            inserter->cycle++;

            Vec2 position = *ecs->Get<PositionComponent>(entity); 
            IVec2 tilePos = position.floorToIVec();

            // adjust for rotations
            auto rotatable = ecs->Get<RotatableComponent>(entity);
            if (rotatable->rotated) {
                float rotation = ecs->Get<RotationComponent>(entity)->degrees;
                rotateInserter(tilePos, inserter, rotation, chunkmap);
            }

            Entity inputEntity = inserter->inputTile->entity;
            Entity outputEntity = inserter->outputTile->entity;
            if (inserter->cycle >= inserter->cycleLength) {
                if (ecs->EntityLives(inputEntity) && ecs->EntityLives(outputEntity)) {
                    if (ecs->entityComponents(inputEntity.id)[getID<InventoryComponent>()] && ecs->entityComponents(outputEntity.id)[getID<InventoryComponent>()]) {
                        Inventory inputInventory = ecs->Get<InventoryComponent>(inputEntity)->inventory;
                        Inventory outputInventory = ecs->Get<InventoryComponent>(outputEntity)->inventory;
                        inputInventory.addItemStack(outputInventory.takeStack());
                    }
                }
                inserter->cycle = 0;
            }
        });
    }

private:
    void rotateInserter(IVec2 tilePos, InserterComponent* inserter, float rotation, ChunkMap& chunkmap) {
        
        int roundedRotation = (int)round(rotation / 90) % 4;

        // find which of the 4 directions the inserter is facing and get the tile offset,
        // multiplied by the inserter's reach to get the input and output tiles

        int dist = (roundedRotation < 2) ? inserter->reach : -inserter->reach;
        bool whichDir = (roundedRotation % 2);
        IVec2 tileOffset = {dist * !whichDir, dist * whichDir};

        IVec2 inputPos = {tilePos.x + tileOffset.x, tilePos.y + tileOffset.y};
        IVec2 outputPos = {tilePos.x - tileOffset.x, tilePos.y - tileOffset.y};

        Tile* inputTile = getTileAtPosition(chunkmap, inputPos);
        Tile* outputTile = getTileAtPosition(chunkmap, outputPos);

        // update tiles to reflect new rotation

        inserter->inputTile = inputTile;
        inserter->outputTile = outputTile;

        // rotations in degrees with their rounded rotation numbers and vector representation
        // 45 < r < 135 : 0 : {0, -1}
        // 135 < r < 225 : 1, -3 : {-1, 0}
        // 225 < r < 270 : 2, -2 : {0, 1}
        // 270 < r < 315 : 3, -1 {1, 0}

    }
};

class TransportSystem : public EntitySystem {
public:
    TransportSystem() {}

    bool Query(ComponentFlags signature) const {
        return signature[getID<TransporterEC>()];
    }

    void Update(ECS* ecs) {
        ForEach([&](Entity entity){
            
        });
    }
};

class DyingSystem : public EntitySystem {
public:
    DyingSystem() : EntitySystem() {}

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<DyingComponent>()]);
    }

    void Update(ECS* ecs) {
        ForEach([&](Entity entity){
            int* timeToRemoval = &ecs->Get<DyingComponent>(entity)->timeToRemoval;
            (*timeToRemoval)--;
            if (*timeToRemoval < 1) {
                ecs->ScheduleRemove(entity);
            }
        });
    }
};

#endif