#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ECS/Entity.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ArchetypePool.hpp"
#include "memory/BlockAllocator.hpp"

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
    static constexpr bool IsComponentArray = true;

    ComponentArray() : data(nullptr) {}

    ComponentArray(C* data) : data(data) {}

    typename std::conditional<std::is_const_v<C>, const C&, C&>::type& operator[](int index) {
        assert(data);
        return data[index];
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

struct JobData {
    EntityCommandBuffer* commandBuffer;
private:
    ArchetypePool* pool;

    int indexBegin; // number of entities before this

    bool initFailed = false;

    using AdditionalExecutionFunc = std::function<void(void* data, int startN, int endN)>;
    std::vector<AdditionalExecutionFunc> additionalExecutions;
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
        assert(neededComponentIndex != -1 && "Job group does not have this component set!");
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

    using JobExecutionMethodType = void(IJob::*)(int N);

    template<class Component, class DoExecutionMethod>
    void setOptionalExecution(DoExecutionMethod additionalExecution) {
        setOptionalExecutionReal(Component::ID, (JobExecutionMethodType)additionalExecution);
    }

private:
    void setOptionalExecutionReal(ComponentID hasComponent, JobExecutionMethodType additionalExecutionPtr) {
        if (pool->archetype.getIndex(hasComponent) != -1) {
            additionalExecutions.push_back([additionalExecutionPtr](void* jobPtr, int startN, int endN){
                for (int N = startN; N < endN; N++) {
                    (((IJob*)jobPtr)->*additionalExecutionPtr)(N);
                }
            });
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

    ArrayRef<AdditionalExecutionFunc> getAdditionalExecutions() const {
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


using JobExePtr = void(*)(void*, int startIndex, int endIndex);
using JobInitPtr = void(*)(void*, JobData&);

struct PureJob {
    JobInitPtr init;
    JobExePtr executeRange;

    bool parallelize    : 1;
    bool needMainThread : 1;
    bool blocking       : 1;
};

struct IJob : PureJob {
    static constexpr int NullStage = -1;

    int realSize = -1; // in bytes

    IJob(JobInitPtr init, JobExePtr execute, bool parallelizable, bool mainThread, bool blocking) {
        this->init = init;
        this->executeRange = execute;
        this->parallelize = parallelizable;
        this->needMainThread = mainThread;
        this->blocking = blocking;
    }
};

template<class JobClass>
constexpr auto JobClassExecuteRange = []
    (void* childPtr, int startN, int endN){
        for (int N = startN; N < endN; N++) {
            ((JobClass*)childPtr)->Execute(N);
        }
    };

template<class JobClass>
constexpr auto JobClassInit = [](void* ptr, JobData& data){
        ((JobClass*)ptr)->Init(data);
    };

template<class Inheritor>
struct IJobDecl : IJob {
    IJobDecl(bool parallelizable, bool mainThread, bool blocking) : IJob(JobClassInit<Inheritor>, JobClassExecuteRange<Inheritor>, parallelizable, mainThread, blocking) {
        static_assert(std::is_trivially_copyable_v<Inheritor>, "Job must be copyable with memcpy");
        static_assert(std::is_trivially_destructible_v<Inheritor>, "Job must not need destructors");
        
        static_assert(&Inheritor::Execute != nullptr, "Job must have Execute method");
        static_assert(std::is_same_v<decltype(&Inheritor::Execute), void(Inheritor::*)(int)>, "Incorrect execute function signature!");
        static_assert(&Inheritor::Init != nullptr, "Job must have Init method");
        static_assert(std::is_same_v<decltype(&Inheritor::Init), void(Inheritor::*)(JobData&)>, "Incorrect init function signature!");
    }
};

// struct JobFunctionData {
//     using SimpleExecute = std::function<void(int N)>;
//     SimpleExecute execute;
//     using SimpleInit = std::function<void(JobData&)>;
//     SimpleInit init;
// };
// didnt work out
// IJob makeJob(const std::function<void(JobData&)>& init, const std::function<void(int N)>& execute, bool parallelize) {
//     IJob job = IJob(
//         [](void* data, JobData& jobData){
//             IJob* job = (IJob*)data;
//             JobFunctionData* fakeData = (JobFunctionData*)job->userdata;
//             fakeData->init(jobData);
//         },
//         [](void* data, int start, int end){
//             IJob* job = (IJob*)data;
//             JobFunctionData* fakeData = (JobFunctionData*)job->userdata;
//             for (int i = start; i < end; i++) {
//                 fakeData->execute(i);
//             }
//         }, parallelize, false, false);
//     job.userdata = new JobFunctionData{execute, init};
//     return job;
// }

// returns number of eligible entities
int findEligiblePools(const IComponentGroup& group, const EntityManager& entityManager, std::vector<ArchetypePool*>* eligiblePools);

struct SystemManager {
    std::vector<ISystem*> systems;
    EntityManager* entityManager = nullptr;
    EntityCommandBuffer unexecutedCommands;
    bool allowParallelization = true;
    // should be big enough blocks for almost all jobs
    BlockAllocator<sizeof(IJob) + 32, 8> jobAllocator;
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
    IJob* newJob(const Job& job) {
        IJob* jobPtr = NEW(Job(job), systemManager->jobAllocator);
        jobPtr->realSize = sizeof(Job);
        return jobPtr;
    }

    template<typename Job>
    JobHandle Schedule(const IComponentGroup& group, const Job& job, const DependencyList& dependencyList) {
        JobHandle handle = jobs.size();
        auto jobPtr = newJob(job);
        jobs.push_back({jobPtr, group});
        jobDependencies.push_back({});
        for (JobHandle jobDependency : dependencyList.jobDependencies) {
            AddDependency(handle, jobDependency);
        }
        return handle;
    }

    template<typename Job>
    JobHandle Schedule(const IComponentGroup& group, const Job& job, JobHandle dependency = -1) {
        IJob* jobPtr = newJob(job);
        JobHandle handle = jobs.size();
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

    void Conflict(JobHandle jobA, JobHandle jobB) {
        // TODO: optimize
        AddDependency(jobA, jobB);
    }

    class Thenner {
        ISystem* system;
        std::vector<JobHandle> lastThenList;
    public:
        Thenner(ISystem* system) : system(system) {}

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

    void clearJobs() {
        for (auto& job : jobs) {
            DELETE(job.job, job.job->realSize, systemManager->jobAllocator);
        }
        jobs.clear();
        jobDependencies.clear();
    }

    virtual ~ISystem() {

    }

    int getGroupSize(const IComponentGroup& group) {
        // TODO: optimize to cache results instead of having to go through all pools every time
        return systemManager->getSizeOf(group);
    }

    template<class T>
    MutArrayRef<T> makeTempArray(size_t elementCount) {
        T* allocation = new T[elementCount];
        tempAllocations.push_back(allocation);
        return MutArrayRef(allocation, elementCount);
    }

    template<class T>
    MutArrayRef<T> makeTempGroupArray(const IComponentGroup& group) {
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

void executeSystem(SystemManager&, ISystem*);
void executeSystems(SystemManager&);


// default jobs

// most strict
template<class Child>
struct IJobParallelFor : IJobDecl<Child> {
    IJobParallelFor() : IJobDecl<Child>(true, false, false) {}
};

// less relaxed. allows for standard queues and vectors without worries of parallelism. May or may not be on the main thread
template<class Child>
struct IJobSingleThreaded : IJobDecl<Child> {
    IJobSingleThreaded() : IJobDecl<Child>(false, false, false) {}
};

// run the job strictly on the main thread, single threaded
template<class Child>
struct IJobMainThread : IJobDecl<Child> {
    // param blocking: allow other jobs while this is run or run purely single threaded
    IJobMainThread(bool blocking) : IJobDecl<Child>(false, true, blocking) {}
};

#define JOB_SINGLE_THREADED(name) struct name : IJobSingleThreaded<name>
#define JOB_PARALLEL_FOR(name) struct name : IJobParallelFor<name>
#define JOB_MAIN_THREAD(name) struct name : IJobMainThread<name>

template <class C>
struct CopyComponentArrayJob : IJobParallelFor<CopyComponentArrayJob<C>> {
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

struct CopyEntityArrayJob : IJobParallelFor<CopyEntityArrayJob> {
    Entity* dst;
    const Entity* src;

    CopyEntityArrayJob(Entity* dst)
    : dst(dst) {}

    void Init(JobData& data) {
        src = data.getEntityArray();
    }

    void Execute(int N) {
        dst[N] = src[N];
    }
};

template<class Component>
struct FillComponentsFromArrayJob : IJobParallelFor<FillComponentsFromArrayJob<Component>> {
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
struct InitializeArrayJob : IJobParallelFor<InitializeArrayJob<T>> {
    MutableArrayRef<T> values;

    T value;

    InitializeArrayJob(MutableArrayRef<T> values, const T& initializationValue) : values(values), value(initializationValue) {
        this->values = values;
    }

    void Init(JobData&) {}

    void Execute(int N) {
        values[N] = value;
    }
};


}

}


#endif