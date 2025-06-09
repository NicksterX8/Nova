#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>
#include "ecs.hpp"
#include "My/Vec.hpp"
#include "ECS/Entity.hpp"

namespace EcsSystem {

using Element = ECS::Entity;
using ElementCommandBuffer = GECS::ElementCommandBuffer;

using Signature = GECS::EcsClass::Signature;

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

struct AbstractGroupArray {
    void* data = nullptr;
    bool readonly = false;
    // for component array
    ComponentID componentType = -1;
    // for plain array

    //AbstractGroupArray* group = nullptr;
    // for element array
    bool elementArray = false;
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

    ElementCommandBuffer commands;

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

using ArchetypePool = GECS::EcsClass::ArchetypalComponentManager::ArchetypePool;

// returns number of eligible elements
int findEligiblePools(const IComponentGroup* group, const GECS::ElementManager& elementManager, std::vector<ArchetypePool*>* eligiblePools);

struct SystemManager {
    std::vector<ISystem*> systems;
    const GECS::ElementManager* elementManager = nullptr;
    //std::vector<void*> tempSystemAllocations;
public:
    SystemManager() {}

    SystemManager(const GECS::ElementManager* elementManager) : elementManager(elementManager) {}

    void addSystem(ISystem* system) {
        systems.push_back(system);
    }

    int getSizeOf(const IComponentGroup& group) const {
        return findEligiblePools(&group, *elementManager, nullptr);
    }
};

struct ISystem {
    bool enabled;
    bool flushCommandBuffers = false; // flush command buffers prior to execution

    ElementCommandBuffer commands;

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
            ElementEvent
        } type;

        struct ElementEvent {
            enum Type {
                ElementAdded,
                ElementRemoved
            } type;

            IComponentGroup* group;
        };

        ElementEvent elementEvent;
    } trigger = {
        .type = Trigger::Type::EveryUpdate
    };

    ISystem(SystemManager& manager) : enabled(true), systemManager(&manager) {
        manager.addSystem(this);
    }

    virtual void ScheduleJobs() = 0;

    virtual void BeforeExecution() {}

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

struct ElementArray : AbstractGroupArray {
    ElementArray(IJob* job) {
        this->elementArray = true;
        if (job)
            job->arrays.push_back(this);
    }

    Element operator[](int index) {
        return ((Element*)data)[index];
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

void executeSystems(SystemManager&, GECS::ElementManager&);

inline bool jobsAreConflicting(IJob* jobA, IJob* jobB) {
    const IComponentGroup& groupA = *jobA->group;
    const IComponentGroup& groupB = *jobB->group;
    auto componentWriteConflicts = (groupA.write & groupB.signature) | (groupB.write & groupA.signature);
    if (componentWriteConflicts.any()) return true;
    
    return false;
}   

}


#endif