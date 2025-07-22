#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>
#include "ECS/EntityManager.hpp"
#include "ECS/ArchetypePool.hpp"
#include "ECS/Job.hpp"
#include "ADT/SmallVector.hpp"
#include "llvm/TinyPtrVector.h"
#include "memory/BlockAllocator.hpp"

template<typename EltT>
using TinyPtrVectorVector = SmallVector<llvm::TinyPtrVector<EltT*>>;

namespace ECS {

namespace Systems {

using JobHandle = int;

struct DependencyList {
    SmallVector<JobHandle, 2> jobDependencies;

    DependencyList() {}

    DependencyList(JobHandle job) {
        jobDependencies.push_back(job);
    }

    DependencyList(std::initializer_list<JobHandle> jobs) : jobDependencies(jobs) {

    }

    // no dependencies
    bool none() const {
        return jobDependencies.empty();
    }
};

struct System;

// returns number of eligible entities
int findEligiblePools(Signature required, Signature rejected, const EntityManager& entityManager, std::vector<const ArchetypePool*>* eligiblePools);

struct SystemManager {
    std::vector<System*> systems;

    std::vector<Group> groups;
    
    EntityManager* entityManager = nullptr;
    EntityCommandBuffer unexecutedCommands;
    bool allowParallelization = USE_MULTITHREADING;

    BlockAllocator<128, 16> jobAllocator;
public:
    SystemManager() {}

    SystemManager(EntityManager* entityManager) : entityManager(entityManager) {}

    void addSystem(System* system) {
        systems.push_back(system);
    }

    GroupID getOrMakeGroup(IComponentGroup componentGroup, Group::TriggerType trigger) {
        // try to find a group with the same components & trigger
        Group newGroup{componentGroup, trigger};
        for (GroupID id = 0; id < groups.size(); id++) {
            if (groups[id] == newGroup) {
                // already exists
                return id;
            }
        }

        // none exist yet of same type, make one
        if (trigger & (Group::EntityEntered | Group::EntityExited)) {
            ECS::GroupWatcherType type = 0;
            if (trigger & Group::EntityEntered)
                type |= ECS::GroupWatcherTypes::EnteredGroup;
            if (trigger & Group::EntityExited)
                type |= ECS::GroupWatcherTypes::ExitedGroup;
            newGroup.watcherPool = entityManager->makeWatcher({newGroup.group.signature, newGroup.group.subtract}, type);
        }
        GroupID id = groups.size();
        groups.push_back(newGroup);
        return id;
    }

    Group* getGroup(GroupID id) {
        return &groups[id];
    }
};

struct System {
    EntityCommandBuffer commands;

    // job schedule
    struct ScheduledJob {
        GroupID group;
        Job* job;
        void* args;
    };
    SmallVector<ScheduledJob> jobs;
    TinyPtrVectorVector<System::ScheduledJob> stageJobs;

    SystemManager* systemManager;

    std::vector<System*> systemDependencies;

    static constexpr int NullSystemOrder = -1;

    int systemOrder = NullSystemOrder;
    bool enabled;
    bool flushCommandBuffers = false; // flush command buffers prior to execution

    struct Trigger {
        enum class Type {
            Init,
            EveryUpdate,
            Quit
        } type;
    } trigger = {
        .type = Trigger::Type::EveryUpdate
    };

    System(SystemManager& manager) : systemManager(&manager), enabled(true) {
        manager.addSystem(this);
    }

    virtual void BeforeExecution() {}
    virtual void AfterExecution() {}

    struct JobDependency {
        JobHandle dependent; // dependent needs dependency.
        JobHandle dependency;
    };

    std::vector<std::vector<JobHandle>> jobDependencies;

    template<typename JobT>
    JobHandle Schedule(GroupID group, const JobT& jobt, JobArgPtrs<JobT> args, const DependencyList& dependencyList) {
        Job* job = NEW(JobT(jobt), systemManager->jobAllocator);
        JobHandle handle = jobs.size();
        // we can't know what the size of the args when we free it so just use malloc
        // we could change that, but since we only ever schedule once per job rn, it doesn't matter and we could just leak it if we wanted
        void* argsPtr = new JobArgPtrs<JobT>(args);
        jobs.push_back({group, job, argsPtr});
        jobDependencies.push_back({});
        for (JobHandle jobDependency : dependencyList.jobDependencies) {
            AddDependency(handle, jobDependency);
        }

        return handle;
    }

    template<typename JobT>
    JobHandle Schedule(GroupID group, const JobT& job, JobArgPtrs<JobT> args, JobHandle dependency = -1) {
        return Schedule(group, job, args, DependencyList{dependency});
    }

    template<typename JobT>
    JobHandle Schedule(GroupID group, const JobT& jobt, const DependencyList& dependencyList) {
        static_assert(std::tuple_size_v<JobArgs<JobT>> == 0, "Job requires arguments and none were provided!");

        Job* job = NEW(JobT(jobt), systemManager->jobAllocator);

        JobHandle handle = jobs.size();
        jobs.push_back(ScheduledJob{group, job, new std::tuple<>{}});
        jobDependencies.push_back({});
        for (JobHandle jobDependency : dependencyList.jobDependencies) {
            AddDependency(handle, jobDependency);
        }
        return handle;
    }

    template<typename JobT>
    JobHandle Schedule(GroupID group, const JobT& job, JobHandle dependency = -1) {
        static_assert(std::tuple_size_v<JobArgs<JobT>> == 0, "Job requires arguments and none were provided!");
        return Schedule(group, job, DependencyList{dependency});
    }

    void AddDependency(JobHandle dependent, JobHandle dependency) {
        if (dependent != -1 && dependency != -1) {
            jobDependencies[dependent].push_back(dependency);
        }
    }

    void Conflict(JobHandle jobA, JobHandle jobB) {
        // TODO: optimize
        AddDependency(jobA, jobB);
    }

    template<typename GroupC>
    GroupID MakeGroup(const GroupC& cgroup, Group::TriggerType trigger = Group::EntityInGroup) {
        return systemManager->getOrMakeGroup(cgroup, trigger);
    }

    class Thenner {
        System* system;
        std::vector<JobHandle> lastThenList;
    public:
        Thenner(System* system) : system(system) {}

        Thenner& Then(JobHandle job) {
            for (auto& dependency : lastThenList) {
                system->AddDependency(job, dependency);
            }
            lastThenList.clear();
            lastThenList.push_back(job);
            return *this;
        }

        Thenner& Then(const std::initializer_list<JobHandle>& jobs) {
            for (auto& job : jobs) {
                for (auto& dependency : lastThenList) {
                    system->AddDependency(job, dependency);
                }
            }
            lastThenList = jobs;
            return *this;
        }

        JobHandle handle() const {
            assert(lastThenList.size() <= 1);

            if (lastThenList.size() == 1)
                return lastThenList[0];
            else
                return -1;
        }
    };

    Thenner Do(const std::initializer_list<JobHandle>& jobs) {
        return Thenner(this).Then(jobs);
    }

    Thenner Do(JobHandle job) {
        return Thenner(this).Then(job);
    }

    virtual ~System() {
        for (auto& job : jobs) {
            if (job.args) {
                free(job.args);
            }
            DELETE(job.job, job.job->size, systemManager->jobAllocator);
        }
    }

    // ordering
    void orderAfter(System* otherSystem) {
        this->systemDependencies.push_back(otherSystem);
    }

    void orderBefore(System* otherSystem) {
        otherSystem->systemDependencies.push_back(this);
    }
};

template<typename T>
GroupArray<T>::GroupArray(System* system, GroupID group) {
    system->systemManager->getGroup(group)->addArray(this);
}

struct IBarrier : System {
    IBarrier(SystemManager& manager) : System(manager) {
        flushCommandBuffers = true;
        enabled = false;
    }
};

template<class C>
struct ReadOnly {
    static constexpr bool read = true;
    static constexpr bool write = false;
    static constexpr bool subtract = false;
    using Type = C;
};

template<class C>
struct ReadWrite {
    static constexpr bool read = true;
    static constexpr bool write = true;
    static constexpr bool subtract = false;
    using Type = C;
};

template<class C>
struct Tag {
    static constexpr bool read = false;
    static constexpr bool write = false;
    static constexpr bool subtract = false;
    using Type = C;
};

template<class C>
struct Subtract {
    static constexpr bool read = false;
    static constexpr bool write = false;
    static constexpr bool subtract = true;
    using Type = C;
};

template<class ...Cs>
struct ComponentGroup : IComponentGroup {
    template<class C>
    constexpr void setComponentUses() {
        if (C::read || C::write)
            signature.set(C::Type::ID);
        if (C::read) {
            read.set(C::Type::ID);
        }
        if (C::write) {
            write.set(C::Type::ID);
        }
        if (C::subtract) {
            subtract.set(C::Type::ID);
        }
    }

    constexpr ComponentGroup() {
        int dummy[] = {0, (setComponentUses<Cs>(), 0) ...};
        (void)dummy;
    }
};

void setupSystems(SystemManager&);

void cleanupSystems(SystemManager&);

void executeSystem(SystemManager&, System*, const std::vector<std::vector<const ArchetypePool*>>& groupPools);
void executeSystems(SystemManager&);


// default jobs

// most strict
template<class Derived, class... Cs>
struct JobParallelFor : JobDecl<Derived, Cs...> {
    JobParallelFor() {
        this->mainThread = false;
        this->parallelize = true;
    }
};

// less relaxed. allows for standard queues and vectors without worries of parallelism. May or may not be on the main thread
template<class Derived, class... Cs>
struct JobSingleThreaded : JobDecl<Derived, Cs...> {
    JobSingleThreaded() {
        this->mainThread = false;
        this->parallelize = false;
        this->blocking = false;
    }
};

// run the job strictly on the main thread, single threaded
template<class Derived, class... Cs>
struct JobMainThread : JobDecl<Derived, Cs...> {
    // param blocking: allow other jobs while this is run or run purely single threaded
    JobMainThread() {
        this->blocking = false;
        this->parallelize = false;
        this->mainThread = true;
    }
};

// run the job strictly on the main thread, single threaded
template<class Derived, class... Cs>
struct JobBlocking : JobDecl<Derived, Cs...> {
    // param blocking: allow other jobs while this is run or run purely single threaded
    JobBlocking() {
        this->blocking = true;
        this->parallelize = false;
        this->mainThread = true;
    }
};

template <class C>
struct CopyComponentArrayJob : JobParallelFor<CopyComponentArrayJob<C>> {
    void Execute(int N, ComponentArray<C> src, C* dst) {
        dst[N] = src[N];
    }
};

struct CopyEntityArrayJob : JobParallelFor<CopyEntityArrayJob> {
    Entity* dst;
    const Entity* src;

    void Execute(int N, EntityArray src, Entity* dst) {
        dst[N] = src[N];
    }
};

template<class Component>
struct FillComponentsFromArrayJob : JobParallelFor<FillComponentsFromArrayJob<Component>> {
    void Execute(int N, ComponentArray<Component> dst, Component src) {
        dst[N] = src[N];
    }
};

template<typename T>
struct InitializeArrayJob : JobParallelFor<InitializeArrayJob<T>> {
    void Execute(int N, MutArrayRef<T> dst, T value) {
        dst[N] = value;
    }
};


}

}


#endif
