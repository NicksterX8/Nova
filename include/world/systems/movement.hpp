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

struct UpdateEntityViewBoxChunkPosJob : IJobMainThread<UpdateEntityViewBoxChunkPosJob> {
    UpdateEntityViewBoxChunkPosJob(ChunkMap* chunkmap)
    : IJobMainThread<UpdateEntityViewBoxChunkPosJob>(true), chunkmap(chunkmap) {}

    ComponentArray<const EC::ViewBox> viewboxes;
    ComponentArray<const EC::Position> positions;
    EntityArray entities;

    ChunkMap* chunkmap;

    void Init(JobData& data) {
        data.getComponentArrays(viewboxes, positions);
        data.getEntityArray(entities);
    }

    void Execute(int N) {
        auto viewBox = viewboxes[N];
        auto position = positions[N];
        auto entity = entities[N];

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

template<typename Trigger, typename T>
struct TriggerGroup {

};

struct EntityEnters {

};

struct TriggerSystem {
    SmallVector<Entity> triggeredEntities;
    using TriggerType = int;
    enum {
        EntityEnters = 1,
        EntityExits = 2,
        EntityCreated = 4,
        EntityDestroyed = 8
    };
    TriggerType trigger;

    struct Group {
        ECS::Signature needed = 0;
        ECS::Signature rejected = 0;
    } group;

    TriggerSystem(SystemManager& manager, TriggerType trigger, Group group)
    : trigger(trigger), group(group) {}

    template<typename Job>
    JobHandle Schedule(const Job& job) {
        
    }
};

// struct NewEntityViewBoxSystem : TriggerSystem {
//     constexpr static TriggerGroup<EntityEnters, ComponentGroup<
//         ReadOnly<EC::ViewBox>,
//         ReadOnly<EC::Position>
//     >> group;

//     ChunkMap* chunkmap;

//     NewEntityViewBoxSystem(SystemManager& manager, ChunkMap* chunkmap)
//     : ISystem(manager), chunkmap(chunkmap) {
        
//     }

//     void ScheduleJobs() {
//         ScheduleWhen(group, UpdateEntityViewBoxChunkPosJob());
//     }
// };

template<class... Components>
constexpr ECS::Signature Need() {
    return ECS::getSignature<Components...>();
}

template<class... Components>
constexpr ECS::Signature Reject() {
    return ECS::getSignature<Components...>();
}

struct NewEntityViewBoxSystem : TriggerSystem {
    NewEntityViewBoxSystem(SystemManager& manager, ChunkMap* chunkmap) 
    : TriggerSystem(manager,
        EntityEnters | EntityExits, {Need<EC::ViewBox, EC::Position>()}), chunkmap(chunkmap)
    {}

    ChunkMap* chunkmap;

    void ScheduleJobs() {
        Schedule(UpdateEntityViewBoxChunkPosJob(chunkmap)); 
    }
};

}

}

#endif