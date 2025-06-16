#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ECS/Entity.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ArchetypePool.hpp"

namespace ECS {

namespace System {

struct IComponentGroup {
    Signature read;
    Signature write;
    Signature signature;
    Signature subtract;
};

struct IJob;

struct Task {
    IComponentGroup* group;
    IJob* job;
};

struct ISystem;

struct TaskGroup {
    IComponentGroup* group;
    std::vector<IJob*> jobs;
    ISystem* system;
    bool flushCommandBuffers;
};

using OptionalExecuteType = void (IJob::*)(int index);

struct AbstractGroupArray {
    void* data = nullptr;
    bool readonly = false;
    // for component array
    ComponentID componentType = -1;
    bool optional = false;
    OptionalExecuteType optionalExecute;
    // for entity array
    bool entityArray = false;
};

using JobGroup = const IComponentGroup*;

struct Dependency {
    std::vector<IJob*> jobDependencies;

    Dependency() {}

    Dependency(IJob* job) {
        jobDependencies.push_back(job);
    }

    Dependency(std::initializer_list<IJob*> jobs) : jobDependencies(jobs) {

    }

    // no dependencies
    bool none() const {
        return jobDependencies.empty();
    }
};

struct IJob {
    enum Type {
        Parallel,
        MainThread
    } type;

    EntityCommandBuffer commands;

    std::vector<AbstractGroupArray*> arrays;

    JobGroup group;

    Dependency thisDependentOn;

    static constexpr int NullStage = -1;
    int stage = NullStage;

    IJob(Type type, JobGroup group) : type(type), group(group) {

    }

    virtual void Execute(int N) = 0;

    virtual ~IJob() {}
};

// returns number of eligible entities
int findEligiblePools(const IComponentGroup* group, const EntityManager& entityManager, std::vector<ArchetypePool*>* eligiblePools);

struct SystemManager {
    std::vector<ISystem*> systems;
    EntityManager* entityManager = nullptr;
    //std::vector<void*> tempSystemAllocations;
public:
    SystemManager() {}

    SystemManager(EntityManager* entityManager) : entityManager(entityManager) {}

    void addSystem(ISystem* system) {
        systems.push_back(system);
    }

    int getSizeOf(const IComponentGroup& group) const {
        return findEligiblePools(&group, *entityManager, nullptr);
    }
};

struct ISystem {
    bool enabled;
    bool flushCommandBuffers = false; // flush command buffers prior to execution

    EntityCommandBuffer commands;

    std::vector<void*> tempAllocations;

    // job schedule
    std::vector<IJob*> jobs;

    SystemManager* systemManager;

    std::vector<ISystem*> systemDependencies;
    static constexpr int NullSystemOrder = -1;
    int systemOrder = NullSystemOrder;

    struct Trigger {
        enum class Type {
            EveryUpdate,
            Init,
            EntityEvent
        } type;

        struct EntityEvent {
            enum Type {
                EntityAdded,
                EntityRemoved
            } type;

            IComponentGroup* group;
        };

        EntityEvent entityEvent;
    } trigger = {
        .type = Trigger::Type::EveryUpdate
    };

    ISystem(SystemManager& manager) : enabled(true), systemManager(&manager) {
        manager.addSystem(this);
    }

    virtual void BeforeExecution() {}
    virtual void ScheduleJobs() {}
    virtual void AfterExecution() {}

    IJob* Schedule(IJob* job, const Dependency& dependency) {
        for (IJob* jobDependency : dependency.jobDependencies) {
            AddDependency(job, jobDependency);
        }
        jobs.push_back(job);
        return job;
    }

    IJob* Schedule(IJob* job, IJob* dependency = nullptr) {
        AddDependency(job, dependency);
        jobs.push_back(job);
        return job;
    }

    void AddDependency(IJob* dependent, IJob* dependency) {
        if (dependent && dependency) {
            dependent->thisDependentOn.jobDependencies.push_back(dependency);
            //dependency->dependentOnThis.jobDependencies.push_back(dependency);
        }
    }

    virtual ~ISystem() {

    }

    int getGroupSize(const IComponentGroup& group) {
        return systemManager->getSizeOf(group);
    }

    template<class T>
    T* makeTempGroupArray(const IComponentGroup& group) {
        T* allocation = new T[getGroupSize(group)];
        tempAllocations.push_back(allocation);
        return allocation;
    }

    // ordering
    void orderAfter(ISystem* otherSystem) {
        this->systemDependencies.push_back(otherSystem);
    }

    void orderBefore(ISystem* otherSystem) {
        otherSystem->systemDependencies.push_back(this);
    }
};

struct IBarrier : ISystem {
    IBarrier(SystemManager& manager) : ISystem(manager) {
        flushCommandBuffers = true;
        enabled = false;
    }

    void ScheduleJobs() {}
};

template<class C>
struct ComponentArray : AbstractGroupArray {
    ComponentArray(const ComponentArray<C>& other) = delete;

    ComponentArray(IJob* job) {
        this->componentType = C::ID;
        this->readonly = std::is_const_v<C>;
        if (job)
            job->arrays.push_back(this);
    }

    typename std::conditional<std::is_const_v<C>, const C&, C&>::type& operator[](int index) {
        return ((C*)data)[index];
    }
};

struct EntityArray : AbstractGroupArray {
    EntityArray(IJob* job) {
        this->entityArray = true;
        if (job)
            job->arrays.push_back(this);
    }

    Entity operator[](int index) {
        return ((Entity*)data)[index];
    }
};

template<class C>
struct OptionalComponentArray : ComponentArray<C> {
    template<class OptionalExecuteMethodT>
    OptionalComponentArray(IJob* job, OptionalExecuteMethodT optionalExecute) : ComponentArray<C>(job) {
        this->optional = true;
        this->optionalExecute = static_cast<OptionalExecuteType>(optionalExecute);
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

void executeSystems(SystemManager&);

inline bool jobsAreConflicting(IJob* jobA, IJob* jobB) {
    const IComponentGroup& groupA = *jobA->group;
    const IComponentGroup& groupB = *jobB->group;
    auto componentWriteConflicts = (groupA.write & groupB.signature) | (groupB.write & groupA.signature);
    if (componentWriteConflicts.any()) return true;
    
    return false;
}   


// default jobs

struct IJobParallelFor : IJob {
    IJobParallelFor(JobGroup group) : IJob(IJob::Parallel, group) {}
};

struct IJobSingleThreaded : IJob {
    IJobSingleThreaded(JobGroup group) : IJob(IJob::MainThread, group) {}
};

template <class C>
struct CopyComponentArrayJob : IJobParallelFor {
    C* dst;
    ComponentArray<C> src;

    CopyComponentArrayJob(JobGroup group, Entity* dst)
    : IJobParallelFor(group), dst(dst), src(this) {}

    void Execute(int N) {
        dst[N] = src[N];
    }
};

struct CopyEntityArrayJob : IJobParallelFor {
    Entity* dst;
    EntityArray src;

    CopyEntityArrayJob(JobGroup group, Entity* dst)
    : IJobParallelFor(group), dst(dst), src(this) {}

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<class Component>
struct FillComponentsFromArrayJob : IJob {
    Component* src;
    ComponentArray<const Component> dst;

    FillComponentsFromArrayJob(Component* src) : src(src), dst(this) {}

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<typename T>
struct InitializeArrayJob : IJob {
    MutableArrayRef<T> values;

    T value;

    InitializeArrayJob(MutableArrayRef<T> values, const T& initializationValue) : values(this), value(initializationValue) {
        this->values = values;
    }

    void Execute(int N) {
        values[N] = value;
    }
};


}

}


#endif