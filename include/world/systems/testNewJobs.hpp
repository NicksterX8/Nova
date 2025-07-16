#pragma once

#include "ECS/Job.hpp"

#include "GUI/components.hpp"
#include "world/components/components.hpp"


using namespace ECS::System;
using namespace ECS::System::New;

struct ChunkMap;

struct GroupVars {};


namespace GUI {

struct PositionsArray {
    GroupArray<int> positions;
};

struct MoveJob2 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    MoveJob2(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        USE_GROUP_VARS(PositionsArray);
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
        SetConst<EC::ViewBox>();
    }

    void update(int scale) {
        this->scale = scale;
    }
};

struct MoveJob3 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    USE_GROUP_VARS(PositionsArray);

    MoveJob3(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
    }

    void update(int scale) {
        this->scale = scale;
    }
};

struct NewSystemEx : NewSystem {
    ChunkMap* chunkmap;
    MoveJob2 moveJob;
    PositionsArray positions;

    NewSystemEx(SystemManager& manager, ChunkMap* chunkmap) : NewSystem(manager), chunkmap(chunkmap), moveJob(chunkmap) {
        Group* group = makeGroup(ComponentGroup<
            ReadOnly<EC::ViewBox>
        >(), &positions);
        group->addArray(&positions.positions);
        
        Schedule(group, moveJob);
    }

    void Update(int scale) {
        moveJob.update(scale);
    }
};

inline void runJob(const ECS::EntityManager& components, Job job, Group* group) {
    int groupSize = /* get group size */ 300;
    
    // for each group, make sure array is big enough to fit one element per entity
    for (auto& array : group->arrays) {
        void*& data = *array.ptrToData;
        data = realloc(data, array.typeSize * groupSize);
    }

    JobDataNew jobData;
    jobData.dependencies = job.dependencies;
    jobData.groupVars = group->vars;
    jobData.indexBegin = 0;
    

    std::vector<ECS::ArchetypePool*> eligiblePools;
    findEligiblePools(group->group, components, &eligiblePools);

    jobData.pool = eligiblePools[0];

    job.executeFunc(&jobData, 0, jobData.pool->size);
}


}