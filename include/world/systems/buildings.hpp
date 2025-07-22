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

    struct ShootJob : JobParallelFor<ShootJob> {
        void Execute(int N, ComponentArray<EC::Gun> gun) {

        }
    } shootJob;

    GunSystem(SystemManager& manager) : System(manager) {
        Schedule(group, shootJob);
    }
};

}

}