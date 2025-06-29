#ifndef WORLD_SYSTEMS_MOVEMENT_INCLUDED
#define WORLD_SYSTEMS_MOVEMENT_INCLUDED

#include "world/EntityWorld.hpp"
#include "ECS/system.hpp"
#include "world/components/components.hpp"
#include "world/functions.hpp"

namespace World {

namespace Systems {

using namespace ECS::System;

JOB_PARALLEL_FOR(FindChangedEntitiesJob) {
    ComponentArray<const EC::Position> positions;
    ComponentArray<const EC::Dynamic> dynamics;

    MutArrayRef<bool> posChanged;
    // TODO: replace with threadsafe queue

    FindChangedEntitiesJob(MutArrayRef<bool> posChangedArray) 
    : posChanged(posChangedArray) {}

    void Init(JobData& data) {
        data.getComponentArray(positions);
        data.getComponentArray(dynamics);
    }

    void Execute(int N) {
        posChanged[N] = positions[N].vec2() != dynamics[N].pos;
    }
};

JOB_SINGLE_THREADED(ChangeEntityPosJob) {
    ComponentArray<EC::Position> oldPositions;
    ComponentArray<const EC::Dynamic> newPositions;
    ComponentArray<const EC::ViewBox> viewboxes;
    const Entity* entities;

    ArrayRef<bool> posChanged;

    ChunkMap* chunkmap;

    ChangeEntityPosJob(ArrayRef<bool> posChangedArray, ChunkMap* chunkmap)
    : posChanged(posChangedArray), chunkmap(chunkmap) {}

    void Init(JobData& data) {
        data.getComponentArrays(oldPositions, newPositions, viewboxes);
        entities = data.getEntityArray();
    }

    void Execute(int N) {
        if (LLVM_UNLIKELY(posChanged[N])) {
            auto oldPosition = oldPositions[N];
            auto newPosition = newPositions[N];
            oldPositions[N].x = newPosition.pos.x;
            oldPositions[N].y = newPosition.pos.y;
            auto viewbox = viewboxes[N];
            World::entityViewChanged(chunkmap, entities[N], newPosition.pos, oldPosition.vec2(), viewbox.box, viewbox.box, false);
        }
    }
};

struct DynamicEntitySystem : ISystem {
    constexpr static ComponentGroup<
        ReadWrite<EC::Position>,
        ReadWrite<EC::Dynamic>,
        ReadOnly<EC::ViewBox>
    > dynamicGroup;

    ChunkMap* chunkmap;

    MutArrayRef<bool> entityPosChanged;

    DynamicEntitySystem(SystemManager& manager, ChunkMap* chunkmap)
    : ISystem(manager), chunkmap(chunkmap) {}

    void alloc() {

    }

    void ScheduleJobs() {
        auto posChangedArray = makeTempGroupArray<bool>(dynamicGroup);
        Schedule(dynamicGroup, ChangeEntityPosJob(posChangedArray, chunkmap), 
            Schedule(dynamicGroup, FindChangedEntitiesJob(posChangedArray)));
    }
};

}

}

#endif