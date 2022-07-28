#ifndef SYSTEMS_INCLUDED
#define SYSTEMS_INCLUDED

#include <vector>
#include <random>
#include "../ECS/ECS.hpp"
#include "../ECS/EntitySystem.hpp"
#include "../SECS/Entity.hpp"

#include "../Log.hpp"
#include "../constants.hpp"
#include "../Entities/Methods.hpp"

static std::default_random_engine randomGen;

/*

class PositionSystem : public EntitySystem {
public:
    ChunkMap* chunkmap = NULL;

    PositionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Position>(Read);
        sys.GiveAccess<EC::Size>(Read);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Position>()]);
    }

    void Update() {
        ForEach<EC::Position>([&](auto entity){
            if (sys.EntityHas<EC::Size>(entity)) {
                return;
            }
            Vec2 position = *sys.GetReadOnly<EC::Position>(entity);
            IVec2 chunkPosition = toChunkPosition(position);
            ChunkData* chunkdata = chunkmap->getChunkData(chunkPosition);
            if (chunkdata) {
                chunkdata->closeEntities.push_back(entity);
            }
        });

        ForEach<EC::Position, EC::Size>([&](auto entity){
            Vec2 position = *sys.GetReadOnly<EC::Position>(entity);
            Vec2 size = sys.GetReadOnly<EC::Size>(entity)->vec2();

            IVec2 minChunkPosition = toChunkPosition(position - size.scaled(0.5f));
            IVec2 maxChunkPosition = toChunkPosition(position + size.scaled(0.5f));
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
        sys.GiveAccess<EC::Rotation>(ReadWrite);
        sys.GiveAccess<EC::AngleMotion>(Read | Remove);
        sys.GiveAccess<EC::Render>(ReadWrite);

        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Rotation>()]);
    }

    void Update() {
        setEntitiesRenderRotation();
    }

private:

    void setEntitiesRenderRotation() {
        ForEach<EC::AngleMotion, EC::Rotation>([&](EntityT<EC::AngleMotion, EC::Rotation> entity){
            float* rotation = &entity.Get<EC::Rotation>()->degrees;
            auto angleMotion = entity.Get<EC::AngleMotion>();
            if (fabs(*rotation - angleMotion->rotationTarget) < angleMotion->rotationSpeed) {
                // reached target
                *rotation = angleMotion->rotationTarget;
                // commandBuffer->RemoveComponent<EC::AngleMotion>(entity);
                return;
            }

            *rotation += angleMotion->rotationSpeed;
        });

        ForEach<EC::Rotation, EC::Render>([&](auto entity){
            // copy rotation to render component
            sys.GetReadWrite<EC::Render>(entity)->rotation = sys.GetReadOnly<EC::Rotation>(entity)->degrees;
        });
    }
};

class RotatableSystem : public EntitySystem {
public:
    RotatableSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Rotatable>(ReadWrite);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Rotatable>()]);
    }

    void Update() {
        ForEach([&](auto entity){
            sys.GetReadWrite<EC::Rotatable>(entity)->rotated = false;
        });
    }
};

class GrowthSystem : public EntitySystem {
public:
    GrowthSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Growth>(Read | Write);
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Growth>()]);
    }

    void Update() {
        ForEach([&](auto entity){
            sys.GetReadWrite<EC::Growth>(entity)->growthValue += 1;
        });
    }
private:

};
*/
class MotionSystem {
public:
    void Update(const EntityWorld& ecs, ChunkMap* chunkmap) {
        ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Motion, EC::Position> > >(
        [&](auto entity){
            auto positionComponent = ecs.Get<EC::Position>(entity);
            Vec2 position = positionComponent->vec2();
            Vec2 target = ecs.Get<EC::Motion>(entity)->target;
            Vec2 delta = {target.x - position.x, target.y - position.y};
            float speed = ecs.Get<EC::Motion>(entity)->speed;
            Vec2 unit = delta.norm().scaled(speed);

            if (delta.length() < speed) {
                positionComponent->x = target.x;
                positionComponent->y = target.y;
            } else {
                positionComponent->x += unit.x;
                positionComponent->y += unit.y;
            }

            entityPositionChanged(chunkmap, &ecs, entity.template cast<EC::Position>(), position);
        });
    }
};
/*

class FollowSystem : public EntitySystem {
public:
    FollowSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(EC::Position) = ReadWrite;
        SYSTEM_ACCESS(EC::Follow) = ReadWrite;
        SYSTEM_ACCESS(EC::Health) = ReadWrite;
    }

    bool Query(ComponentFlags signature) const {
        return (signature[getID<EC::Follow>()]);
    }

    void Update() {
        ForEach<EC::Position, EC::Follow>([&](EntityT<EC::Position, EC::Follow> entity){
            auto followComponent = sys.GetReadWrite<EC::Follow>(entity);
            if (!sys.EntityExists(followComponent->entity)) {
                return;
            }
            EntityT<EC::Position> following = followComponent->entity;
            Vec2 target = *sys.GetReadOnly<EC::Position>(following);

            EC::Position* position = sys.GetReadWrite<EC::Position>(entity);
            Vec2 delta = {target.x - position->x, target.y - position->y};
            Vec2 unit = delta.norm().scaled(followComponent->speed);

            if (delta.length() < followComponent->speed) {
                position->x = target.x;
                position->y = target.y;

                // do something
                // hurt them if they have health
                if (following.Has<EC::Health>()) {
                    following.Get<EC::Health>()->healthValue -= 10;
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
        sys.GiveAccess<EC::Health>(ReadWrite);
        sys.GiveAccess<EC::Immortal>(Flag);

        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Health>()]);
    }

    void Update() {
        ForEach([&](Entity entity){
            float* health = &sys.GetReadWrite<EC::Health>(entity)->healthValue;
            // Must do check like this instead of (*health <= 0.0f) to account for NaN values,
            // which can occur when infinite damage is done to an entity with infinite health
            // so in that situation the infinite damage wins out, rather than the infinte health
            if (!(*health > 0.0f)) {
                if (!sys.entityComponents(entity.id)[getID<EC::Immortal>()])
                    DestroyEntity(entity);
            }
        });
    }
};

class InventorySystem : public EntitySystem {
public:
    InventorySystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        sys.GiveAccess<EC::Growth>(ReadWrite);
        sys.GiveAccess<EC::Inventory>(ReadWrite);

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Inventory>()]);
    }

    void Update() {
        ForEach<EC::Growth>([&](auto entity){
            EC::Growth* growth = sys.GetReadWrite<EC::Growth>(entity);
            if (growth->growthValue > 100) {
                growth->growthValue -= 100;
                Inventory* inventory = &sys.GetReadWrite<EC::Inventory>(entity)->inventory;
                inventory->addItemStack(ItemStack(Items::Wall));
            }
        });
    }
};

class ExplosivesSystem : public EntitySystem {
public:
    ExplosivesSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(EC::Explosive) = ReadWrite;
        SYSTEM_ACCESS(EC::Position) = Read;
        SYSTEM_ACCESS(EC::Motion) = Read;

        mustExecuteAfterStructuralChanges = true;
        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Explosive>()]);
    }

    void Update() {
        ForEach<EC::Position, EC::Motion>([&](auto entity){
            Vec2 target = sys.GetReadOnly<EC::Motion>(entity)->target;
            Vec2 position = *sys.GetReadOnly<EC::Position>(entity);

            if (target.x == position.x && target.y == position.y) {
                // EXPLODE
                
                commandBuffer->DeferredStructuralChange([entity, target](ECS* ecs){
                    Entity explosion = ecs->New();
                    auto explosionComponent = *ecs->Get<EC::Explosive>(entity)->explosion;
                    ecs->Add<EC::Explosion>(explosion, explosionComponent);
                    ecs->Add<EC::Position>(explosion, EC::Position(target));

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
                            EC::Render(Textures.Tiles.sand, RenderLayer::Particles),
                            EC::Motion(particleTarget, speed)
                        );

                        ecs->Add<EC::Rotation>(particle, {angle});
                        ecs->Add<EC::AngleMotion>(particle, EC::AngleMotion(angle + dist * 30, speed));
                    }
                });

                commandBuffer->DeferredStructuralChange([entity](ECS* ecs){
                    ecs->Destroy(entity);
                });
                
            }
        });
    }

};

class ExplosionSystem : public EntitySystem {
public:
    ExplosionSystem(ECS* ecs) : EntitySystem(ecs) {
        using namespace ComponentAccess;
        SYSTEM_ACCESS(EC::Position) = Read;
        SYSTEM_ACCESS(EC::Size) = Read;
        SYSTEM_ACCESS(EC::Explosion) = ReadWrite;
        SYSTEM_ACCESS(EC::Health) = ReadWrite;
        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags signature) const {
        return (signature[getID<EC::Explosion>()]);
    }

    void Update() {}

    void Update(ECS* ecs, ChunkMap& chunkmap) {
        ForEach<EC::Position, EC::Explosion>([&](auto entity){
            EC::Explosion* explosion = ecs->Get<EC::Explosion>(entity);
            float radius = explosion->radius;
            //EC::Position* positionComponent = ecs->Get<EC::Position>(entity);
            //Vec2 position = {positionComponent->x, positionComponent->y};
            Vec2 position = *sys.GetReadOnly<EC::Position>(entity);
            
            // search for entities to kill
            forEachEntityInRange(ecs, &chunkmap, position, radius, [&](Entity affectedEntity){
                if (sys.entityComponents(affectedEntity.id)[getID<EC::Health>()]) {
                    if (affectedEntity == entity) {
                        // explosion component shouldn't affect itself
                        return 0;
                    }

                    Vec2 aPos = *sys.GetReadOnly<EC::Position>(affectedEntity);
                    if (sys.entityComponents(affectedEntity.id)[getID<EC::Size>()]) {
                        auto size = sys.GetReadOnly<EC::Size>(affectedEntity);
                        aPos.x += size->width/2;
                        aPos.y += size->height/2;
                    }
                    Vec2 delta = aPos - position;
                    float distanceSqrd = delta.x * delta.x + delta.y * delta.y;
                    
                    if (distanceSqrd < radius*radius) {
                        // entity is in range of explosion,
                        // reduce their health
                        sys.GetReadWrite<EC::Health>(affectedEntity)->healthValue -= explosion->damage;
                    }
                }
                return 0;
            });
            
            explosion->life--;
            if (explosion->life < 1) {
                commandBuffer->DeferredStructuralChange([entity](ECS* ecs){
                    ecs->Destroy(entity);
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
        sys.GiveAccess<EC::Inserter>(Read | Write);
        sys.GiveAccess<EC::Inventory>(Read | Write);
        sys.GiveAccess<EC::Position>(Read);
        sys.GiveAccess<EC::Rotation>(Read);
        sys.GiveAccess<EC::Rotatable>(Read);

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        ComponentFlags need = componentSignature<
            EC::Inserter, EC::Position, EC::Rotation, EC::Rotatable
        >();
        return ((entitySignature & need) == need);
    }

    void Update() {
        if (!chunkmap) {
            Log.Error("InserterSystem::Update Chunkmap is NULL");
            return;
        }

        ForEach([&](Entity entity){
            auto inserter = sys.GetReadWrite<EC::Inserter>(entity);
            inserter->cycle++;

            Vec2 position = *sys.GetReadOnly<EC::Position>(entity); 
            IVec2 tilePos = position.floorToIVec();

            // adjust for rotations
            auto rotatable = sys.GetReadOnly<EC::Rotatable>(entity);
            if (rotatable->rotated) {
                float rotation = sys.GetReadOnly<EC::Rotation>(entity)->degrees;
                rotateInserter(tilePos, inserter, rotation);
            }

            if (inserter->cycle >= inserter->cycleLength) {
                Tile* inputTile = getTileAtPosition(*chunkmap, inserter->inputTile);
                Tile* outputTile = getTileAtPosition(*chunkmap, inserter->outputTile);
                if (inputTile && outputTile) {
                    EntityT<> inputEntity = inputTile->entity;
                    EntityT<> outputEntity = outputTile->entity;
                    if (inputEntity.Has<EC::Inventory>() && outputEntity.Has<EC::Inventory>()) {
                        Inventory inputInventory = sys.GetReadWrite<EC::Inventory>(inputEntity)->inventory;
                        Inventory outputInventory = sys.GetReadWrite<EC::Inventory>(outputEntity)->inventory;
                        inputInventory.addItemStack(outputInventory.removeItemStack(outputInventory.firstItemStack()));
                    }
                    inserter->cycle = 0;
                }
            }
        });
    }

private:
    void rotateInserter(IVec2 tilePos, EC::Inserter* inserter, float rotation) {
        
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
        SYSTEM_ACCESS(EC::Transporter) = Read | Write;

        mustExecuteAfterStructuralChanges = true;
    }

    bool Query(ComponentFlags signature) const {
        return signature[getID<EC::Transporter>()];
    }

    void Update() {
        ForEach([&](Entity entity){
            
        });
    }
};

class DyingSystem : public EntitySystem {
public:
    DyingSystem(ECS* ecs) : EntitySystem(ecs) {
        SYSTEM_ACCESS(EC::Dying) = ComponentAccess::ReadWrite;
        containsStructuralChanges = true;
    }

    bool Query(ComponentFlags entitySignature) const {
        return (entitySignature[getID<EC::Dying>()]);
    }

    void Update() {
        ForEach([&](Entity entity){
            int* timeToRemoval = &sys.GetReadWrite<EC::Dying>(entity)->timeToRemoval;
            (*timeToRemoval)--;
            if (*timeToRemoval < 1) {
                DestroyEntity(entity);
            }
        });
    }
};

*/

#endif