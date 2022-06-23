#ifndef SYSTEMS_INCLUDED
#define SYSTEMS_INCLUDED

#include <vector>
#include <random>
#include "../ECS/ECS.hpp"
#include "../ECS/EntitySystem.hpp"
#include "../ECS/EntityType.hpp"

#include "../Log.hpp"
#include "../constants.hpp"

static std::default_random_engine randomGen;

class PositionSystem : public EntitySystem {
public:
    ChunkMap* chunkmap = NULL;

    PositionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<PositionComponent>(Read);
        sys.GiveAccess<SizeComponent>(Read);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<PositionComponent>()]);
    }

    void Update() {
        ForEach<PositionComponent>([&](auto entity){
            if (sys.EntityHas<SizeComponent>(entity)) {
                return;
            }
            Vec2 position = *sys.GetReadOnly<PositionComponent>(entity);
            IVec2 chunkPosition = tileToChunkPosition(position);
            ChunkData* chunkdata = chunkmap->getChunkData(chunkPosition);
            if (chunkdata) {
                chunkdata->closeEntities.push_back(entity);
            }
        });

        ForEach<PositionComponent, SizeComponent>([&](auto entity){
            Vec2 position = *sys.GetReadOnly<PositionComponent>(entity);
            Vec2 size = sys.GetReadOnly<SizeComponent>(entity)->toVec2();

            IVec2 minChunkPosition = tileToChunkPosition(position);
            IVec2 maxChunkPosition = tileToChunkPosition(position + size);
            for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
                for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                    IVec2 chunkPosition = {col, row};
                    ChunkData* chunkdata = chunkmap->getChunkData(chunkPosition);
                    if (chunkdata) {
                        chunkdata->closeEntities.push_back(entity);
                    }
                }
            }
        });
    }

private:

};

class RotationSystem : public EntitySystem {
public:
    RotationSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<RotationComponent>(ReadWrite);
        sys.GiveAccess<AngleMotionEC>(Read | Remove);
        sys.GiveAccess<RenderComponent>(ReadWrite);

        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<RotationComponent>()]);
    }

    void Update() {
        setEntitiesRenderRotation();
    }

private:

    void setEntitiesRenderRotation() {
        ForEach<AngleMotionEC, RotationComponent>([&](EntityType<AngleMotionEC, RotationComponent> entity){
            float* rotation = &entity.Get<RotationComponent>()->degrees;
            auto angleMotion = entity.Get<AngleMotionEC>();
            if (fabs(*rotation - angleMotion->rotationTarget) < angleMotion->rotationSpeed) {
                // reached target
                *rotation = angleMotion->rotationTarget;
                // commandBuffer->RemoveComponent<AngleMotionEC>(entity);
                return;
            }

            *rotation += angleMotion->rotationSpeed;
        });

        ForEach<RotationComponent, RenderComponent>([&](auto entity){
            // copy rotation to render component
            sys.GetReadWrite<RenderComponent>(entity)->rotation = sys.GetReadOnly<RotationComponent>(entity)->degrees;
        });
    }
};

class RotatableSystem : public EntitySystem {
public:
    RotatableSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<RotatableComponent>(ReadWrite);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<RotatableComponent>()]);
    }

    void Update() {
        ForEach([&](auto entity){
            sys.GetReadWrite<RotatableComponent>(entity)->rotated = false;
        });
    }
};

class GrowthSystem : public EntitySystem {
public:
    GrowthSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<GrowthComponent>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<GrowthComponent>()]);
    }

    void Update() {
        ForEach([&](auto entity){
            sys.GetReadWrite<GrowthComponent>(entity)->growthValue += 1;
        });
    }
private:

};

class MotionSystem : public EntitySystem {
public:
    MotionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<MotionComponent>(Read | Remove);
        sys.GiveAccess<PositionComponent>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<MotionComponent>()]);
    }

    void Update() {
        ForEach<PositionComponent, MotionComponent>([&](auto entity){
            PositionComponent* position = sys.GetReadWrite<PositionComponent>(entity);
            Vec2 target = sys.GetReadOnly<MotionComponent>(entity)->target;
            Vec2 delta = {target.x - position->x, target.y - position->y};
            float speed = sys.GetReadOnly<MotionComponent>(entity)->speed;
            Vec2 unit = delta.norm().scaled(speed);

            if (delta.length() < speed) {
                position->x = target.x;
                position->y = target.y;
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
    FollowSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(PositionComponent) = ReadWrite;
        SYSTEM_ACCESS(FollowComponent) = ReadWrite;
        SYSTEM_ACCESS(HealthComponent) = ReadWrite;
    }

    bool Query(ComponentFlags signature) const {
        return (signature[getID<FollowComponent>()]);
    }

    void Update() {
        ForEach<PositionComponent, FollowComponent>([&](EntityType<PositionComponent, FollowComponent> entity){
            auto followComponent = sys.GetReadWrite<FollowComponent>(entity);
            if (!sys.EntityExists(followComponent->entity)) {
                return;
            }
            EntityType<PositionComponent> following = followComponent->entity;
            Vec2 target = *sys.GetReadOnly<PositionComponent>(following);

            PositionComponent* position = sys.GetReadWrite<PositionComponent>(entity);
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
    HealthSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<HealthComponent>(ReadWrite);
        sys.GiveAccess<ImmortalEC>(Flag);

        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<HealthComponent>()]);
    }

    void Update() {
        ForEach([&](Entity entity){
            float* health = &sys.GetReadWrite<HealthComponent>(entity)->healthValue;
            // Must do check like this instead of (*health <= 0.0f) to account for NaN values,
            // which can occur when infinite damage is done to an entity with infinite health
            // so in that situation the infinite damage wins out, rather than the infinte health
            if (!(*health > 0.0f)) {
                if (!sys.entityComponents(entity.id)[getID<ImmortalEC>()])
                    DestroyEntity<COMPONENTS>(entity);
            }
        });
    }
};

class InventorySystem : public EntitySystem {
public:
    InventorySystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<GrowthComponent>(ReadWrite);
        sys.GiveAccess<InventoryComponent>(ReadWrite);

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<InventoryComponent>()]);
    }

    void Update() {
        ForEach<GrowthComponent>([&](auto entity){
            GrowthComponent* growth = sys.GetReadWrite<GrowthComponent>(entity);
            if (growth->growthValue > 100) {
                growth->growthValue -= 100;
                Inventory* inventory = &sys.GetReadWrite<InventoryComponent>(entity)->inventory;
                inventory->addItemStack(ItemStack(Items::Wall));
            }
        });
    }
};

class ExplosivesSystem : public EntitySystem {
public:
    ExplosivesSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(ExplosiveComponent) = ReadWrite;
        SYSTEM_ACCESS(PositionComponent) = Read;
        SYSTEM_ACCESS(MotionComponent) = Read;

        mustExecuteAfterStructuralChanges = true;
        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<ExplosiveComponent>()]);
    }

    void Update() {
        ForEach<PositionComponent, MotionComponent>([&](auto entity){
            Vec2 target = sys.GetReadOnly<MotionComponent>(entity)->target;
            Vec2 position = *sys.GetReadOnly<PositionComponent>(entity);

            if (target.x == position.x && target.y == position.y) {
                // EXPLODE
                
                commandBuffer->DeferredStructuralChange([entity, target](ECS* ecs){
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
                });

                commandBuffer->DeferredStructuralChange([entity](ECS* ecs){
                    ecs->Remove<COMPONENTS>(entity);
                });
                
            }
        });
    }

};

class ExplosionSystem : public EntitySystem {
public:
    ExplosionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(PositionComponent) = Read;
        SYSTEM_ACCESS(SizeComponent) = Read;
        SYSTEM_ACCESS(ExplosionComponent) = ReadWrite;
        SYSTEM_ACCESS(HealthComponent) = ReadWrite;
        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags signature) const {
        return (signature[getID<ExplosionComponent>()]);
    }

    void Update() {}

    void Update(ECS* ecs, ChunkMap& chunkmap) {
        ForEach<PositionComponent, ExplosionComponent>([&](auto entity){
            ExplosionComponent* explosion = ecs->Get<ExplosionComponent>(entity);
            float radius = explosion->radius;
            //PositionComponent* positionComponent = ecs->Get<PositionComponent>(entity);
            //Vec2 position = {positionComponent->x, positionComponent->y};
            Vec2 position = *sys.GetReadOnly<PositionComponent>(entity);
            
            // search for entities to kill
            forEachEntityInRange(ecs, &chunkmap, position, radius, [&](Entity affectedEntity){
                if (sys.entityComponents(affectedEntity.id)[getID<HealthComponent>()]) {
                    if (affectedEntity == entity) {
                        // explosion component shouldn't affect itself
                        return 0;
                    }

                    Vec2 aPos = *sys.GetReadOnly<PositionComponent>(affectedEntity);
                    if (sys.entityComponents(affectedEntity.id)[getID<SizeComponent>()]) {
                        auto size = sys.GetReadOnly<SizeComponent>(affectedEntity);
                        aPos.x += size->width/2;
                        aPos.y += size->height/2;
                    }
                    Vec2 delta = aPos - position;
                    float distanceSqrd = delta.x * delta.x + delta.y * delta.y;
                    
                    if (distanceSqrd < radius*radius) {
                        // entity is in range of explosion,
                        // reduce their health
                        sys.GetReadWrite<HealthComponent>(affectedEntity)->healthValue -= explosion->damage;
                    }
                }
                return 0;
            });
            
            explosion->life--;
            if (explosion->life < 1) {
                commandBuffer->DeferredStructuralChange([entity](ECS* ecs){
                    ecs->Remove<COMPONENTS>(entity);
                });
            }
        });
    }
};

class InserterSystem : public EntitySystem {
public:
    ChunkMap* chunkmap = NULL;

    InserterSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<InserterComponent>(Read | Write);
        sys.GiveAccess<InventoryComponent>(Read | Write);
        sys.GiveAccess<PositionComponent>(Read);
        sys.GiveAccess<RotationComponent>(Read);
        sys.GiveAccess<RotatableComponent>(Read);

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        ComponentFlags need = componentSignature<
            InserterComponent, PositionComponent, RotationComponent, RotatableComponent
        >();
        return ((entitySignature & need) == need);
    }

    void Update() {
        if (!chunkmap) {Log.Error("InserterSystem::Update Chunkmap is NULL");}

        ForEach([&](Entity entity){
            auto inserter = sys.GetReadWrite<InserterComponent>(entity);
            inserter->cycle++;

            Vec2 position = *sys.GetReadOnly<PositionComponent>(entity); 
            IVec2 tilePos = position.floorToIVec();

            // adjust for rotations
            auto rotatable = sys.GetReadOnly<RotatableComponent>(entity);
            if (rotatable->rotated) {
                float rotation = sys.GetReadOnly<RotationComponent>(entity)->degrees;
                rotateInserter(tilePos, inserter, rotation);
            }

            EntityType<> inputEntity = getTileAtPosition(*chunkmap, inserter->inputTile)->entity;
            EntityType<> outputEntity = getTileAtPosition(*chunkmap, inserter->outputTile)->entity;
            if (inserter->cycle >= inserter->cycleLength) {
                if (inputEntity.Exists() && outputEntity.Exists()) {
                    if (inputEntity.Has<InventoryComponent>() && outputEntity.Has<InventoryComponent>()) {
                    //if (sys.entityComponents(inputEntity.Unwrap().id)[getID<InventoryComponent>()] && sys.entityComponents(outputEntity.id)[getID<InventoryComponent>()]) {
                        Inventory inputInventory = sys.GetReadWrite<InventoryComponent>(inputEntity)->inventory;
                        Inventory outputInventory = sys.GetReadWrite<InventoryComponent>(outputEntity)->inventory;
                        inputInventory.addItemStack(outputInventory.removeItemStack(outputInventory.firstItemStack()));
                    }
                }
                inserter->cycle = 0;
            }
        });
    }

private:
    void rotateInserter(IVec2 tilePos, InserterComponent* inserter, float rotation) {
        
        int roundedRotation = (int)round(rotation / 90) % 4;

        // find which of the 4 directions the inserter is facing and get the tile offset,
        // multiplied by the inserter's reach to get the input and output tiles

        int dist = (roundedRotation < 2) ? inserter->reach : -inserter->reach;
        bool whichDir = (roundedRotation % 2);
        IVec2 tileOffset = {dist * !whichDir, dist * whichDir};

        IVec2 inputPos = {tilePos.x + tileOffset.x, tilePos.y + tileOffset.y};
        IVec2 outputPos = {tilePos.x - tileOffset.x, tilePos.y - tileOffset.y};

        // unnecessary
        // Tile* inputTile = getTileAtPosition(*chunkmap, inputPos);
        // Tile* outputTile = getTileAtPosition(*chunkmap, outputPos);

        // update tiles to reflect new rotation

        inserter->inputTile = inputPos;
        inserter->outputTile = outputPos;

        // rotations in degrees with their rounded rotation numbers and vector representation
        // 45 < r < 135 : 0 : {0, -1}
        // 135 < r < 225 : 1, -3 : {-1, 0}
        // 225 < r < 270 : 2, -2 : {0, 1}
        // 270 < r < 315 : 3, -1 {1, 0}

    }
};



class TransportSystem : public EntitySystem {
public:
    TransportSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(TransporterEC) = Read | Write;

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags signature) const {
        return signature[getID<TransporterEC>()];
    }

    void Update() {
        ForEach([&](Entity entity){
            
        });
    }
};

class DyingSystem : public EntitySystem {
public:
    DyingSystem(ECS* ecs) : EntitySystem(ecs) {
        SYSTEM_ACCESS(DyingComponent) = ComponentAccess::ReadWrite;
        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<DyingComponent>()]);
    }

    void Update() {
        ForEach([&](Entity entity){
            int* timeToRemoval = &sys.GetReadWrite<DyingComponent>(entity)->timeToRemoval;
            (*timeToRemoval)--;
            if (*timeToRemoval < 1) {
                DestroyEntity<COMPONENTS>(entity);
            }
        });
    }
};

#endif