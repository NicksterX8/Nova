#pragma once

#include "utils/map.h"
#include "utils/common-macros.hpp"
#include "ECS/system.hpp"

namespace ECS {

namespace System {

namespace New {

struct Job {
    JobExePtr executeFunc;
    Signature readComponents = 0;
    Signature writeComponents = 0;
    void* dependencies;
    bool parallelizable;
    bool enabled = true;

    struct ConditionalExecution {
        ECS::Signature required;
        JobExePtr execute;
    };

    std::vector<ConditionalExecution> conditionalExecutions;

    Job(bool parallelizable = false) : parallelizable(parallelizable) {}

    template<typename D>
    Job(bool parallelizable, const D& dependencies) : parallelizable(parallelizable) {
        setDependencies(dependencies);
    }

    void addConditionalExecute(ECS::Signature requiredSignature, JobExePtr exeFunc) {
        conditionalExecutions.push_back({requiredSignature, exeFunc});
    }

    template<typename D>
    void SetDependencies(const D& val) {
        dependencies = new D(val);
    }

    template<class... Cs>
    void SetConst() {
        writeComponents &= ~ECS::getSignature<Cs...>();
    }
};

struct JobDataNew {
    void* dependencies;
    void* groupVars;

    ArchetypePool* pool;
    int indexBegin;

    template<class Component>
    Component* getComponentArray() {
        char* poolComponentArray = pool->getComponentArray(Component::ID);
        assert(poolComponentArray && "Archetype pool does not have this component!");

        // need to adjust to make the pointer point 'componentIndex' number of components behind itself,
        // so when indexBegin is added to the base index in the for loop,
        // the range is actually 0...chunkSize
        poolComponentArray -= (indexBegin * sizeof(Component));
        return (Component*)poolComponentArray;
    }
};

struct GroupArrayI {};

template<typename T>
struct GroupArray : GroupArrayI {
private:
    T* data = nullptr;
public:
    T& operator[](int index){
        return data[index];
    }

    friend struct Group;
};

struct GroupArrayT {
    void** ptrToData;
    size_t capacity; // num elements
    size_t typeSize;
};

struct Group {
    IComponentGroup group;
    void* vars;
    std::vector<GroupArrayT> arrays;

    Group(IComponentGroup group, void* vars) : group(group), vars(vars) {}

    template<typename T>
    void addArray(GroupArray<T>* array) {
        GroupArrayT voidArray;
        voidArray.ptrToData = (void**)&array->data;
        voidArray.typeSize = sizeof(T);
        voidArray.capacity = 0;
        arrays.push_back(voidArray);
    }
};

struct NewSystem {
    struct ScheduledJob {
        Group* group;
        Job job;
    };
    
    SmallVector<ScheduledJob, 0> jobs;

    NewSystem(SystemManager& manager) {}

    

    template<typename JobT>
    JobHandle Schedule(Group* group, const JobT& jobt) {
        Job job = jobt;
        Signature illegalReads = job.readComponents & ~group->group.read;
        if (illegalReads.any()) {
            LogError("Illegal job component reads!");
        }
        Signature illegalWrites = job.writeComponents & ~group->group.write;
        if (illegalWrites.any()) {
            LogError("Illegal job component writes!");
        }

        jobs.push_back(ScheduledJob{group, job});
        return 0;
    }

    template<typename GroupC>
    Group* makeGroup(const GroupC& cgroup, void* groupVars) {
        Group* group = new Group(cgroup, groupVars);
        return group;
    }
};

#define NAMESPACED_COMPONENT_ID_SIGNATURE(component, namespace) ECS::Signature{namespace::component::ID}

#define GET_COMPONENT_SIGNATURE(namespace, ...) MAP_LIST_UD_OR(NAMESPACED_COMPONENT_ID_SIGNATURE, namespace, __VA_ARGS__)


#define DECL_COMPONENT_ARRAY(component) EC::component* component = jobData->getComponentArray<EC::component>();

// #define DECL_COMPONENT_ARRAY(component) EC::component* COMBINE(component, Array) = jobData->getComponentArray<EC::component>();
// #define DECL_COMPONENT_REF_FROM_ARRAY(component) EC::component& component = COMBINE(component, Array)[N];

// #define EXECUTE_START_FUNC(job, ...) \
//     { \
//         using namespace EC; \
//         job.readComponents = ECS::getSignature<__VA_ARGS__>(); \
//         job.writeComponents = ECS::getSignature<__VA_ARGS__>(); \
//     } \
//     job.executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
//     JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
//     GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
//     Dependencies& deps = *(Dependencies*)jobData->dependencies; \
//     MAP(DECL_COMPONENT_ARRAY2, __VA_ARGS__) \
//     for (int N = _startIndex; N < _endIndex; N++) {
// #define EXECUTE_END_FUNC } };

#define EXECUTE_START_OBJECT_CLASS(job, ...) \
    { \
        using namespace EC; \
        job.readComponents = ECS::getSignature<__VA_ARGS__>(); \
        job.writeComponents = ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    job.dependencies = this; \
    job.executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY2, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
#define EXECUTE_END_OBJECT_CLASS } }; }

// #define EXECUTE_START_CLASS4 \
//     { \
//     using ThisClass = std::remove_reference_t<decltype(*this)>; \
//     this->dependencies = this; \
//     this->executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
//     JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
//     GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
//     ThisClass& self = *(ThisClass*)jobData->dependencies; \
//     for (int N = _startIndex; N < _endIndex; N++) {
// #define EXECUTE_END_CLASS4 } }; }

#define EXECUTE_START_CLASS(...) \
    { \
        using namespace EC; \
        this->readComponents = ECS::getSignature<__VA_ARGS__>(); \
        this->writeComponents = ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    this->dependencies = this; \
    this->executeFunc = [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
#define EXECUTE_END_CLASS } }; }

#define USE_GROUP_VARS(groupvars) using GroupVars = groupvars;

// params: components needed to run
#define EXECUTE_IF_START(...) { \
        using namespace EC; \
        this->readComponents |= ECS::getSignature<__VA_ARGS__>(); \
        this->writeComponents |= ECS::getSignature<__VA_ARGS__>(); \
    } \
    { \
    using ThisClass = std::remove_reference_t<decltype(*this)>; \
    this->addConditionalExecute(GET_COMPONENT_SIGNATURE(EC, __VA_ARGS__), [](void* jobDataPtr, int _startIndex, int _endIndex){\
    JobDataNew* jobData = (JobDataNew*)jobDataPtr; \
    GroupVars& groupVars = *(GroupVars*)jobData->groupVars; \
    ThisClass& self = *(ThisClass*)jobData->dependencies; \
    MAP(DECL_COMPONENT_ARRAY, __VA_ARGS__) \
    for (int N = _startIndex; N < _endIndex; N++) {
        
#define EXECUTE_IF_END } }); }


} // end namespace New

} // end namespace System

} // end namespace ECS