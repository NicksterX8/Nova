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
        // Disabled this because i realized the group shouldn't determine the permissions of a job/system, that needs to be 
        // entirely separate. Since that information doesn't really benefit us at all rn anyways, i've decided to not make a
        // permissions system yet.
        // Signature illegalReads = job.readComponents & ~group->group.read;
        // if (illegalReads.any()) {
        //     LogError("Illegal job component reads!");
        // }
        // Signature illegalWrites = job.writeComponents & ~group->group.write;
        // if (illegalWrites.any()) {
        //     LogError("Illegal job component writes!");
        // }

        // we can't know what the size of the args when we free it so just use malloc
        // we could change that, but since we only ever schedule once per job rn, it doesn't matter and we could just leak it if we wanted
        void* argsPtr = malloc(sizeof(JobArgs<JobT>));
        new (argsPtr) JobArgs<JobT>(args);

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

    void destroy() {
        for (auto& job : jobs) {
            if (job.args) {
                free(job.args);
            }
        }
    }
};

}

}

}