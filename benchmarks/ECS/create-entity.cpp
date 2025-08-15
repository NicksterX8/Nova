#include "bench.hpp"

#include "world/EntityWorld.hpp"
#include "world/components/components.hpp"

using namespace ECS;

int main() {
    
    const int ITERATIONS = 10000;

    START_TIME(initWorld);

    for (int i = 0; i < ITERATIONS; i++) {
        EntityWorld ecs;
        ecs.init();
    }

    END_TIME(initWorld);

    PRINT_TIME(initWorld, ITERATIONS);
}