#pragma once

#include "ECS/System.hpp"
#include "world/components/components.hpp"

namespace World {

namespace Systems {

using namespace ECS::Systems;

struct GunSystem : System {
    GroupID group = MakeGroup(ComponentGroup<
        ReadWrite<EC::Gun>,
        ReadOnly<EC::Position>,
    >());

    struct ShootJob : JobParallelFor<ShootJob, EC::Gun> {
        void Execute(int N) {
            auto gun = Get<EC::AngleMotion>(N);
            Entity entity = GetEntity(N);
            commandBuffer->addComponent(entity, EC::Dying{5});
        }
    } shootJob;

    GunSystem(SystemManager& manager) : System(manager) {
        Schedule(group, shootJob);
    }
};

}

}