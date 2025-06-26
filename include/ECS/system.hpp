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

using JobGroup = const IComponentGroup*;
using JobHandle = int;

struct DependencyList {
    std::vector<JobHandle> jobDependencies;

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

template<class C>
struct ComponentArray {
private:
    C* data;
public:
    ComponentArray() : data(nullptr) {}

    ComponentArray(C* data) : data(data) {}

    typename std::conditional<std::is_const_v<C>, const C&, C&>::type& operator[](int index) {
        assert(data);
        return ((C*)data)[index];
    }
};

struct EntityArray {
private:
    const Entity* data;
public:
    EntityArray() : data(nullptr) {}

    EntityArray(const Entity* data) : data(data) {}

    Entity operator[](int index) {
        assert(data);
        return ((const Entity*)data)[index];
    }
};

struct JobChunk {
    IJob* job;
    ArchetypePool* pool;
    int indexBegin;
    int indexEnd; // exclusive
    int poolOffset;
};

 using JobExecutionMethodType = void (IJob::*)(int index);

struct JobData {
    EntityCommandBuffer* commandBuffer;
private:
    ArchetypePool* pool;

    int indexBegin; // number of entities before this

    bool initFailed = false;

    std::vector<JobExecutionMethodType> additionalExecutions;
public:

    explicit JobData(const JobChunk& jobChunk, EntityCommandBuffer* commandBuffer) : pool(jobChunk.pool), indexBegin(jobChunk.indexBegin) {

    }

    template<class... Components>
    void getComponentArrays(ComponentArray<Components>&... arrays) {
        FOR_EACH_VAR_TYPE(getComponentArray(arrays));
    }

    template<class Component>
    Component* getComponentArray() {
        constexpr ComponentID neededComponent = Component::ID;
        constexpr bool constArray = std::is_const_v<Component>;
        int neededComponentIndex = pool->archetype.getIndex(neededComponent);
        assert(neededComponentIndex != -1 && "Couldn't get component array");
        char* poolComponentArray = pool->getBuffer(neededComponentIndex);
        Uint16 componentSize = pool->archetype.sizes[neededComponentIndex];
        // need to adjust to make the pointer point 'componentIndex' number of components behind itself,
        // so when indexBegin is added to the base index in the for loop,
        // the range is actually 0...chunkSize
        poolComponentArray -= (indexBegin * componentSize);
        return (Component*)poolComponentArray;
    }

    template<class Component>
    void getComponentArray(ComponentArray<Component>& array) {
        array = getComponentArray<Component>();
    }

    template<class Component, class DoExecutionMethod>
    void setOptionalExecution(DoExecutionMethod additionalExecution) {
        setOptionalExecutionReal(Component::ID, (JobExecutionMethodType)additionalExecution);
    }

private:
    void setOptionalExecutionReal(ComponentID hasComponent, JobExecutionMethodType additionalExecution) {
        if (pool->archetype.getIndex(hasComponent) != -1) {
            additionalExecutions.push_back(additionalExecution);
        }
    }
public:

    template<class Component>
    void getComponentArrayMaybe(ComponentArray<Component>& array) {
        constexpr ComponentID neededComponent = Component::ID;
        constexpr bool constArray = std::is_const_v<Component>;
        int neededComponentIndex = pool->archetype.getIndex(neededComponent);
        if (neededComponentIndex == -1) {
            array = nullptr;
        }
        char* poolComponentArray = pool->getBuffer(neededComponentIndex);
        Uint16 componentSize = pool->archetype.sizes[neededComponentIndex];
        // need to adjust to make the pointer point 'componentIndex' number of components behind itself,
        // so when indexBegin is added to the base index in the for loop,
        // the range is actually 0...chunkSize
        poolComponentArray -= (indexBegin * componentSize);
        array = (Component*)poolComponentArray;
    }

    const Entity* getEntityArray() {
        const Entity* entities = pool->entities;
        entities -= indexBegin;
        return entities;
    }

    void getEntityArray(EntityArray& entityArray) {
        entityArray = getEntityArray();
    }

    EntityCommandBuffer* getCommandBuffer() {
        return commandBuffer;
    }

    ArrayRef<JobExecutionMethodType> getAdditionalExecutions() const {
        return additionalExecutions;
    }
    

    bool failed() const {
        return initFailed;
    }

    // call in Init() to indicate an error in initalization, and the job will not be executed
    void error(const char* errMsg) {
        initFailed = true;
        LogError("Job init error: %s", errMsg);
    }
};

struct MinimalJob {
    EntityCommandBuffer commandBuffer;

    using ExecutionMethodType = void(*)(int Index);
};

struct ExperimentalAny {
    void* buffer;

    template<typename T>
    ExperimentalAny(const T& value) {
        buffer = new T(value);
    }

    template<typename T>
    const T& read() const {
        return *(T*)buffer;
    }

    template<typename T>
    T& read() {
        return *(T*)buffer;
    }
};

struct IJob {
    static constexpr int NullStage = -1;

    int stage = NullStage;
    int realSize = -1; // in bytes

    bool parallelize    : 1;
    bool needMainThread : 1;
    bool blocking       : 1;

    IJob(bool parallelizable, bool mainThread, bool blocking) : parallelize(parallelizable), needMainThread(mainThread), blocking(blocking) {

    }

    virtual void Execute(int N) = 0;

    virtual void Init(JobData& data) = 0;

    virtual ~IJob() {}
};

// returns number of eligible entities
int findEligiblePools(const IComponentGroup& group, const EntityManager& entityManager, std::vector<ArchetypePool*>* eligiblePools);

struct SystemManager {
    std::vector<ISystem*> systems;
    EntityManager* entityManager = nullptr;
    EntityCommandBuffer unexecutedCommands;
public:
    SystemManager() {}

    SystemManager(EntityManager* entityManager) : entityManager(entityManager) {}

    void addSystem(ISystem* system) {
        systems.push_back(system);
    }

    int getSizeOf(const IComponentGroup& group) const {
        return findEligiblePools(group, *entityManager, nullptr);
    }
};

struct ISystem {
    bool enabled;
    bool flushCommandBuffers = false; // flush command buffers prior to execution

    EntityCommandBuffer commands;

    std::vector<void*> tempAllocations;

    // job schedule
    struct ScheduledJob {
        IJob* job;
        IComponentGroup group;
    };
    std::vector<ScheduledJob> jobs;

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

    struct JobDependency {
        JobHandle dependent; // dependent needs dependency.
        JobHandle dependency;
    };

    std::vector<std::vector<JobHandle>> jobDependencies;

    template<typename Job>
    JobHandle Schedule(const IComponentGroup& group, const Job& job, const DependencyList& dependencyList) {
        JobHandle handle = jobs.size();
        Job* jobPtr = new Job(job);
        jobPtr->realSize = sizeof(Job);
        jobs.push_back({jobPtr, group});
        jobDependencies.push_back({});
        for (JobHandle jobDependency : dependencyList.jobDependencies) {
            AddDependency(handle, jobDependency);
        }
        return handle;
    }

    template<typename Job>
    JobHandle Schedule(const IComponentGroup& group, const Job& job, JobHandle dependency = -1) {
        Job* jobPtr = new Job(job);
        return Schedule(group, jobPtr, dependency);
    }

    template<typename Job>
    JobHandle Schedule(const IComponentGroup& group, Job* jobPtr, JobHandle dependency = -1) {
        JobHandle handle = jobs.size();
        jobPtr->realSize = sizeof(Job);
        jobs.push_back({jobPtr, group});
        jobDependencies.push_back({});
        if (dependency != -1) {
            AddDependency(handle, dependency);
        }
        return handle;
    }

    void AddDependency(JobHandle dependent, JobHandle dependency) {
        if (dependent != -1 && dependency != -1) {
            jobDependencies[dependent].push_back(dependency);
        }
    }

    void clearJobs() {
        jobs.clear();
        jobDependencies.clear();
    }

    virtual ~ISystem() {

    }

    int getGroupSize(const IComponentGroup& group) {
        return systemManager->getSizeOf(group);
    }

    template<class T>
    T* makeTempArray(size_t elementCount) {
        T* allocation = new T[elementCount];
        tempAllocations.push_back(allocation);
        return allocation;
    }

    template<class T>
    T* makeTempGroupArray(const IComponentGroup& group) {
        return makeTempArray<T>(getGroupSize(group));
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

// inline bool jobsAreConflicting(IJob* jobA, IJob* jobB) {
//     const IComponentGroup& groupA = *jobA->group;
//     const IComponentGroup& groupB = *jobB->group;
//     auto componentWriteConflicts = (groupA.write & groupB.signature) | (groupB.write & groupA.signature);
//     if (componentWriteConflicts.any()) return true;
    
//     return false;
// }   


// default jobs

// most strict
struct IJobParallelFor : IJob {
    IJobParallelFor() : IJob(true, false, false) {}
};

// less relaxed. allows for standard queues and vectors without worries of parallelism. May or may not be on the main thread
struct IJobSingleThreaded : IJob {
    IJobSingleThreaded() : IJob(false, false, false) {}
};

// run the job strictly on the main thread, single threaded
struct IJobMainThread : IJob {
    // param blocking: allow other jobs while this is run or run purely single threaded
    IJobMainThread(bool blocking) : IJob(false, true, blocking) {}
};

template <class C>
struct CopyComponentArrayJob : IJobParallelFor {
    C* dst;
    ComponentArray<C> src;

    CopyComponentArrayJob(Entity* dst)
    : dst(dst) {}

    void Init(JobData& data) {
        data.getComponentArrays(src);
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

struct CopyEntityArrayJob : IJobParallelFor {
    Entity* dst;
    const Entity* src;

    CopyEntityArrayJob(JobGroup group, Entity* dst)
    : dst(dst) {}

    void Init(JobData& data) {
        src = data.getEntityArray();
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<class Component>
struct FillComponentsFromArrayJob : IJobParallelFor {
    const Component* src;
    ComponentArray<Component> dst;

    FillComponentsFromArrayJob(Component* src) : src(src) {}

    void Init(JobData& data) {
        dst = data.getComponentArray<Component>();
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<typename T>
struct InitializeArrayJob : IJobParallelFor {
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