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

struct FindChangedEntitiesJob : JobParallelFor<FindChangedEntitiesJob> {
    void Execute(int N, ComponentArray<EC::Position> positions, ComponentArray<EC::Dynamic> dynamics, GroupArray<bool> posChanged) {
        posChanged[N] = positions[N].vec2() != dynamics[N].pos;
    }
};


struct ChangeEntityPosJob : JobSingleThreaded<ChangeEntityPosJob> {
    ChunkMap* chunkmap;

    ChangeEntityPosJob(ChunkMap* chunkmap)
    : chunkmap(chunkmap) {}

    void Execute(int N, EntityArray entities,
        ComponentArray<EC::Position> oldPositions, ComponentArray<const EC::Dynamic> newPositions,
        ComponentArray<const EC::ViewBox> viewboxes, 
        GroupArray<const bool> posChanged)
    {
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

struct SetChunkMapPosJob : JobSingleThreaded<SetChunkMapPosJob> {
    ChunkMap* chunkmap;
    EntityWorld* ecs;

    SetChunkMapPosJob(ChunkMap* chunkmap, EntityWorld* ecs) : chunkmap(chunkmap), ecs(ecs) {}

    void Execute(int N, EntityArray entity) {
        if (!ecs->EntityExists(entity[N])) return;
        // entityViewChanged(chunkmap, entity[N], position[N].vec2(), position[N].vec2(), viewbox[N].box, viewbox[N].box, true);
        auto position = *ecs->Get<EC::Position>(entity[N]);
        auto viewbox = *ecs->Get<EC::ViewBox>(entity[N]);
        entityViewChanged(chunkmap, entity[N], position.vec2(), position.vec2(), viewbox.box, viewbox.box, true);
    }
};

struct DynamicEntitySystem : System {
    GroupID dynamicGroup = MakeGroup(ComponentGroup<
        ReadWrite<EC::Position>,
        ReadWrite<EC::Dynamic>,
        ReadOnly<EC::ViewBox>
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

    DynamicEntitySystem(SystemManager& manager, ChunkMap* chunkmap, EntityWorld* ecs)
    : System(manager), chunkmap(chunkmap), setChunkMapPos(chunkmap, ecs) {
        Schedule(dynamicGroup, changeEntityPos, entityPosChanged.refConst(), 
            Schedule(dynamicGroup, findChangedEntities, &entityPosChanged));

       Schedule(newEntities, setChunkMapPos);        
    }
};

struct UpdateEntityViewBoxChunkPosJob : JobBlocking<UpdateEntityViewBoxChunkPosJob> {
    UpdateEntityViewBoxChunkPosJob(ChunkMap* chunkmap)
    : chunkmap(chunkmap) {}

    EntityArray entities;

    ChunkMap* chunkmap;


    void Execute(int N, ComponentArray<const EC::ViewBox> viewboxes, ComponentArray<const EC::Position> positions) {
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



}

}

#endif