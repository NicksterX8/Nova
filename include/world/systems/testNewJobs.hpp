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

    void Execute(int N, ComponentArray<EC::ViewBox> viewbox, GroupArray<int>* positionsOut, int x) {
        positionsOut[0][N] = viewbox[N].box.min.x + counter++;
    }
};

struct NewEntityViewBoxSystem : NewSystem {
    Group* enterGroup = makeGroup(ComponentGroup<
        ReadOnly<EC::ViewBox>
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