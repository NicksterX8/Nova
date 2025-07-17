#pragma once

#include "ECS/system.hpp"
#include "ADT/SmallVector.hpp"

namespace ECS {

namespace System {

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
        EntityDestroyed = 8,
        EntityInGroup = 16
    };
    TriggerType trigger;

    struct Group {
        ECS::Signature needed = 0;
        ECS::Signature rejected = 0;
    } group;

    TriggerSystem(SystemManager& manager, TriggerType trigger, Group group)
    : trigger(trigger), group(group) {}

    template<typename JobT>
    JobHandle Schedule(const JobT& job) {
        return 0;
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



}

}