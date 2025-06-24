#ifndef WORLD_SYSTEMS_MOVEMENT_INCLUDED
#define WORLD_SYSTEMS_MOVEMENT_INCLUDED

#include "world/EntityWorld.hpp"
#include "ECS/system.hpp"
#include "world/components/components.hpp"
#include "world/functions.hpp"

namespace World {

namespace Systems {

using namespace ECS::System;

struct FindChangedEntitiesJob : IJobParallelFor {
    ComponentArray<const EC::Position> positions;
    ComponentArray<const EC::Dynamic> dynamics;

    bool* posChanged;
    // TODO: replace with threadsafe queue

    FindChangedEntitiesJob(JobGroup group, bool* posChangedArray) 
    : IJobParallelFor(group), positions(this), dynamics(this), posChanged(posChangedArray) {

    }

    void Execute(int N) {
        posChanged[N] = positions[N].vec2() != dynamics[N].pos;
    }
};

struct ChangeEntityPosJob : IJobSingleThreaded {
    ComponentArray<EC::Position> oldPositions;
    ComponentArray<const EC::Dynamic> newPositions;
    ComponentArray<const EC::ViewBox> viewboxes;
    EntityArray entities;

    bool* posChanged;

    ChunkMap* chunkmap;

    ChangeEntityPosJob(JobGroup group, bool* posChangedArray, ChunkMap* chunkmap)
    : IJobSingleThreaded(group), oldPositions(this), newPositions(this), viewboxes(this), entities(this), posChanged(posChangedArray), chunkmap(chunkmap) {}

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

    DynamicEntitySystem(SystemManager& manager, ChunkMap* chunkmap)
    : ISystem(manager), chunkmap(chunkmap) {
        
    }

    void ScheduleJobs() {
        bool* posChangedArray = makeTempGroupArray<bool>(dynamicGroup);
        Schedule(new ChangeEntityPosJob(&dynamicGroup, posChangedArray, chunkmap), 
            Schedule(new FindChangedEntitiesJob(&dynamicGroup, posChangedArray)));
    }
};

}

}

#endif