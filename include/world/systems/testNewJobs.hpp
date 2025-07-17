#pragma once

#include "ECS/Job.hpp"
#include "ECS/NewSystem.hpp"

#include "GUI/components.hpp"
#include "world/components/components.hpp"


using namespace ECS::System;
using namespace ECS::System::New;

struct ChunkMap;

namespace World {

struct MoveJob4 : JobDeclNew<MoveJob4> {
    int counter = 0;

    MoveJob4() {
        addConditionalExecute<&MoveJob4::HealthExecute>();
    }

    void Execute(int N, ComponentArray<const EC::ViewBox> viewbox, GroupArray<int>* positionsOut, int x) {
        positionsOut[0][N] = viewbox[N].box.min.x + counter++;
        LogInfo("non health execute");
    }

    void HealthExecute(int N, ComponentArray<const EC::Health> health, GroupArray<int>* positionsOut, int x) {
        if (health[N].health > 80.0f) {
            LogOnce(Info, "HEALTHY!!");
        }
        LogInfo("health execute %d", x);
    }
};

struct NewEntityViewBoxSystem : NewSystem {
    Group* enterGroup = makeGroup(ComponentGroup<
        ReadOnly<EC::ViewBox>,
        ReadOnly<EC::Health>
    >());

    GroupArray<int> enterGroupVars{enterGroup};

    MoveJob4 moveJob4;

    int scale = 10;

    NewEntityViewBoxSystem(SystemManager& manager, ChunkMap* chunkmap) 
    : NewSystem(manager) {
        Schedule(enterGroup, moveJob4, {&enterGroupVars, 5});

        // Group* exitGroup = makeGroup(ComponentGroup<
        //     ReadOnly<EC::ViewBox>
        // >(), Group::EntityExits);
        // Schedule(exitGroup, moveJob);
    }
};



}