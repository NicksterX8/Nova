#ifndef WORLD_SYSTEMS_MOVEMENT_INCLUDED
#define WORLD_SYSTEMS_MOVEMENT_INCLUDED

#include "world/EntityWorld.hpp"
#include "ECS/System.hpp"
#include "world/components/components.hpp"
#include "world/functions.hpp"
#include "ECS/Job.hpp"
#include "Chunks.hpp"

namespace World {

namespace Systems {

using namespace ECS::Systems;

struct FindChangedEntitiesJob : JobParallelFor<FindChangedEntitiesJob, const EC::Position, const EC::Dynamic> {
    void Execute(int N, GroupArray<bool> posChanged) {
        posChanged[N] = Get<EC::Position>(N).vec2() != Get<EC::Dynamic>(N).pos;
    }
};


struct ChangeEntityPosJob : JobSingleThreaded<ChangeEntityPosJob, const EC::ViewBox, EC::Position, const EC::Dynamic> {
    ChunkMap* chunkmap;

    ChangeEntityPosJob(ChunkMap* chunkmap)
    : chunkmap(chunkmap) {}

    void Execute(int N, GroupArray<const bool> posChanged) {
        if (LLVM_UNLIKELY(posChanged[N])) {
            auto& oldPosition = Get<EC::Position>(N);
            auto newPosition = Get<EC::Dynamic>(N);
            oldPosition.x = newPosition.pos.x;
            oldPosition.y = newPosition.pos.y;
            auto viewbox = Get<EC::ViewBox>(N);
            auto entity = GetEntity(N);
            World::entityViewChanged(chunkmap, entity, newPosition.pos, oldPosition.vec2(), viewbox.box, viewbox.box, false);
        }
    }
};

struct SetChunkMapPosJob : JobSingleThreaded<SetChunkMapPosJob> {
    ChunkMap* chunkmap;
    EntityWorld* ecs;

    SetChunkMapPosJob(ChunkMap* chunkmap, EntityWorld* ecs) : chunkmap(chunkmap), ecs(ecs) {}

    void Execute(int N) {
        auto entity = GetEntity(N);
        if (!ecs->EntityExists(entity)) return;
        // entityViewChanged(chunkmap, entity[N], position[N].vec2(), position[N].vec2(), viewbox[N].box, viewbox[N].box, true);
        auto position = *ecs->Get<EC::Position>(entity);
        auto viewbox = *ecs->Get<EC::ViewBox>(entity);
        entityViewChanged(chunkmap, entity, position.vec2(), position.vec2(), viewbox.box, viewbox.box, true);
    }
};

struct DynamicEntitySystem : System {
    GroupID dynamicGroup = MakeGroup(ComponentGroup<
        ReadWrite<EC::Position>,
        ReadWrite<EC::Dynamic>,
        ReadOnly<EC::ViewBox>
    >());

    GroupID velGroup = MakeGroup(ComponentGroup<
        ReadOnly<EC::Velocity>,
        ReadWrite<EC::Dynamic>
    >());

    GroupID treeGroup = MakeGroup(ComponentGroup<
        ReadWrite<EC::Position>
    >());

    ChunkMap* chunkmap;

    ChangeEntityPosJob changeEntityPos{chunkmap};
    FindChangedEntitiesJob findChangedEntities;

    GroupArray<bool> entityPosChanged{this, dynamicGroup};

    GroupID newEntities = MakeGroup(ComponentGroup<
        ReadWrite<EC::Position>,
        ReadWrite<EC::ViewBox>
    >(), Group::EntityEntered);

    SetChunkMapPosJob setChunkMapPos;

    struct VelocityJob : JobParallelFor<VelocityJob, const EC::Velocity, EC::Dynamic> {
        void Execute(int N) {
            auto vel = Get<EC::Velocity>(N).vel;
            auto& pos = Get<EC::Dynamic>(N).pos;
            pos += vel;
        }
    } velocityJob;

    DynamicEntitySystem(SystemManager& manager, ChunkMap* chunkmap, EntityWorld* ecs)
    : System(manager), chunkmap(chunkmap), setChunkMapPos(chunkmap, ecs) {
        Do(
            Schedule(velGroup, velocityJob)
        ).Then(
            Schedule(dynamicGroup, findChangedEntities, &entityPosChanged)
        ).Then(
            Schedule(dynamicGroup, changeEntityPos, entityPosChanged.refConst())
        ).Then(
            Schedule(newEntities, setChunkMapPos)
        );
    }
};

struct UpdateEntityViewBoxChunkPosJob : JobBlocking<UpdateEntityViewBoxChunkPosJob, const EC::Position, const EC::ViewBox> {
    UpdateEntityViewBoxChunkPosJob(ChunkMap* chunkmap)
    : chunkmap(chunkmap) {}

    ChunkMap* chunkmap;

    void Execute(int N) {
        auto viewBox = Get<EC::ViewBox>(N);
        auto position = Get<EC::Position>(N);
        auto entity = GetEntity(N);

        Vec2 pos = position.vec2();

        IVec2 minChunkPosition = toChunkPosition(pos + viewBox.box.min);
        IVec2 maxChunkPosition = toChunkPosition(pos + viewBox.box.max());
        for (int col = minChunkPosition.x; col <= maxChunkPosition.x; col++) {
            for (int row = minChunkPosition.y; row <= maxChunkPosition.y; row++) {
                IVec2 chunkPosition = {col, row};
                // add entity to new chunk
                ChunkData* newChunkdata = chunkmap->get(chunkPosition);
                if (newChunkdata) {
                    newChunkdata->closeEntities.push(entity);
                }
            }
        }
    }
};



}

}

#endif