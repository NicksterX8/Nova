#include "world/EntityWorld.hpp"
#include "world/components/components.hpp"
#include "ECS/System.hpp"
#include "global.hpp"
#include "utils/bench.hpp"

using namespace ECS::Systems;

namespace EC = World::EC;

struct TestJob : JobDecl<TestJob, EC::Position, EC::Dynamic> {
    void Execute(int N) {
        auto& pos = Get<EC::Position>(N);
        auto& dyn = Get<EC::Dynamic>(N);

        pos.x = dyn.pos.x;
        pos.y = dyn.pos.y;
    }
};

struct TestSystem : System {
    GroupID group = MakeGroup(
        ComponentGroup<
        ReadWrite<EC::Position>,
        ReadWrite<EC::Dynamic>
    >());
    
    TestJob job;

    TestSystem(SystemManager& manager) : System(manager) {
        Schedule(group, job);
    }
};

int main() {
    const int ITERATIONS = 100;
    const int ENTITIES = 30000;

    // Global.threadManager.initThreads(5);

    EntityWorld ecs;
    ecs.init();

    START_TIME(create);
    for (int i = 0; i < ENTITIES; i++) {
        Entity e = ecs.createEntity(-1);
        ecs.Add<EC::Position>(e, Vec2{i, i});
        ecs.Add<EC::Dynamic>(e, {Vec2{-i, -i}});
    }
    END_TIME(create);
    PRINT_TIME(create);

    SystemManager systems{&ecs};
    TestSystem testSystem{systems};

    setupSystems(systems);

    
    START_TIME(executeSystem);
    for (int i = 0; i < ITERATIONS; i++) {
        executeSystems(systems);
    }
    END_TIME(executeSystem);
    PRINT_TIME(executeSystem, ITERATIONS);

    return 0;
}