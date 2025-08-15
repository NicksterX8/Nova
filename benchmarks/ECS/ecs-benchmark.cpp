#include "world/EntityWorld.hpp"
#include "world/components/components.hpp"
#include "ECS/System.hpp"
#include "global.hpp"
#include "bench.hpp"

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
    const int ITERATIONS = 6000;
    const int ENTITIES = 30000;

    // Global.threadManager.initThreads(5);

    EntityWorld ecs;
    ecs.init();

    auto freq = SDL_GetPerformanceFrequency();

    auto startCreate = SDL_GetPerformanceCounter();

    for (int i = 0; i < ENTITIES; i++) {
        Entity e = ecs.createEntity(-1);
        ecs.Add<EC::Position>(e, Vec2{i, i});
        ecs.Add<EC::Dynamic>(e, {Vec2{-i, -i}});
    }

    auto endCreate = SDL_GetPerformanceCounter();

    auto diffCreate = endCreate - startCreate;
    auto msCreate = (double)diffCreate / (double)freq * 1000.0;
    printf("create ms: %f\n", msCreate);

    SystemManager systems{&ecs};
    TestSystem testSystem{systems};

    setupSystems(systems);

    
    auto start = SDL_GetPerformanceCounter();

    for (int i = 0; i < ITERATIONS; i++) {
        executeSystems(systems);
    }

    auto end = SDL_GetPerformanceCounter();

    auto diff = end - start;
    auto ms = (double)diff / (double)freq * 1000.0;

    printf("ms: %f", ms);





    return 0;
}