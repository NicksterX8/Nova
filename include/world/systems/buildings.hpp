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

    struct ShootJob : JobParallelFor<ShootJob, EC::Gun, const EC::Position, const EC::Rotation> {
        EntityWorld* ecs;

        ShootJob(EntityWorld* ecs) : ecs(ecs) {}

        void Execute(int N, Tick tick) {
            auto& gun = Get<EC::Gun>(N);
            auto position = Get<EC::Position>(N);
            auto rotation = Get<EC::Rotation>(N);
            Vec2 dir = get_sincosf(rotation.degrees * M_PI / 180.0f);
            Vec2 delta = dir * gun.projectileSpeed;
            if (tick - gun.lastFired > gun.cooldown) {
                Entity projectile = gun.projectileFired(commandBuffer, position.vec2());
                commandBuffer->addComponent(projectile, EC::Velocity{delta});
                commandBuffer->addComponent(projectile, EC::Rotation(rotation));

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