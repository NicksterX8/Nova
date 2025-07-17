#pragma once

#include "Job.hpp"

namespace ECS {

namespace System {

namespace New {

struct NewSystem {
    struct ScheduledJob {
        Group* group;
        Job job;
        void* args;
    };
    
    SmallVector<ScheduledJob, 0> jobs;


    NewSystem(SystemManager& manager) {}

    template<typename JobT>
    JobHandle Schedule(Group* group, const JobT& jobt, JobArgs<JobT> args) {
        // static_assert(std::is_same_v<JobT::ArgsTuple, JobArgs>, "Incorrect job arguments");

        Job job = jobt;
        Signature illegalReads = job.readComponents & ~group->group.read;
        if (illegalReads.any()) {
            LogError("Illegal job component reads!");
        }
        Signature illegalWrites = job.writeComponents & ~group->group.write;
        if (illegalWrites.any()) {
            LogError("Illegal job component writes!");
        }

        void* argsPtr = new JobArgs<JobT>(args);

        jobs.push_back(ScheduledJob{group, job, argsPtr});
        return 0;
    }

    template<typename GroupC>
    Group* makeGroup(const GroupC& cgroup, void* groupVars = nullptr) {
        Group* group = new Group(cgroup, Group::EntityInGroup);
        return group;
    }

    template<typename GroupC>
    Group* makeGroup(const GroupC& cgroup, Group::TriggerType trigger, void* groupVars = nullptr) {
        Group* group = new Group(cgroup, trigger);
        return group;
    }
};

}

}

}