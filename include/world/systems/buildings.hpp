#pragma once

#include "ECS/System.hpp"
#include "world/components/components.hpp"

namespace World {

namespace Systems {

using namespace ECS::Systems;

struct GunSystem : System {
    GroupID group = MakeGroup(ComponentGroup<
        ReadWrite<EC::Gun>,
        ReadOnly<EC::Position>
    >());

    Tick currentTick = NullTick;

    struct ShootJob : JobParallelFor<ShootJob, EC::Gun, EC::Position> {
        EntityWorld* ecs;

        ShootJob(EntityWorld* ecs) : ecs(ecs) {}

        void Execute(int N, Tick tick) {
            auto gun = Get<EC::Gun>(N);
            auto position = Get<EC::Position>(N);
            if (tick - gun.lastFired > gun.cooldown) {
                Entity projectile = gun.projectileFired(commandBuffer, position.vec2());
                commandBuffer->addComponent(projectile, EC::Motion({100, 100}, 1.0f));

                gun.lastFired = tick;
            }
        }
    } shootJob;

    GunSystem(SystemManager& manager, EntityWorld* ecs) : System(manager), shootJob(ecs) {
        Schedule(group, shootJob, &currentTick);
    }


    void BeforeExecution() override {
        currentTick = Metadata->getTick();
    }
};

}

}