#pragma once

#include "utils/type_traits.hpp"
#include "ECS/Signature.hpp"
#include "ECS/EntityManager.hpp"
#include <vector>

namespace ECS {

namespace Systems {

using JobExePtr = void(*)(void* job, void* vars, int startIndex, int endIndex);

struct Job {
    // void* dependencies = nullptr;
    JobExePtr executeFunc = nullptr;

    Entity* entities;
    void** componentArrays;
    
    EntityCommandBuffer* commandBuffer;

    Uint8 componentIDs[8];
    Signature readComponents = 0;
    Signature writeComponents = 0;

    uint32_t size;

    bool parallelize     : 1;
    bool mainThread      : 1;
    bool blocking        : 1;
    bool enabled         : 1;
    
    struct ConditionalExecution {
        ECS::Signature required;
        JobExePtr execute;
    };

    static constexpr int MaxConditionalExecutions = 4;
    int nConditionalExecutions = 0;
    ConditionalExecution conditionalExecutions[MaxConditionalExecutions];

    Job() {
        parallelize = true;
        mainThread = false;
        blocking = false;
        enabled = true;

        memset(componentIDs, 255, sizeof(componentIDs));
    }
};

struct JobData {
    void* dependencies;
    void* groupVars;

    const ArchetypePool* pool;
    int indexBegin;

    EntityCommandBuffer* commandBuffer;

    template<class Component>
    Component* getComponentArray() const {
        char* poolComponentArray = pool->getComponentArray(Component::ID);
        assert(poolComponentArray && "Archetype pool does not have this component!");

        // need to adjust to make the pointer point 'componentIndex' number of components behind itself,
        // so when indexBegin is added to the base index in the for loop,
        // the range is actually 0...chunkSize
        poolComponentArray -= (indexBegin * sizeof(Component));
        return (Component*)poolComponentArray;
    }

    template<class... Components>
    bool hasComponents() const {
        return pool->signature().hasComponents<Components...>();
    }

    Entity* getEntityArray() const {
        return pool->entities - indexBegin;
    }
};

struct Group;
struct System;

using GroupID = int;

template<typename T>
struct GroupArray {
    T* data = nullptr;
public:
    GroupArray(System* system, GroupID group);

    GroupArray(GroupArray<std::remove_const_t<T>> mut) : data(mut.data) {}

    std::conditional_t<std::is_const_v<T>, const T&, T&> operator[](int index) const {
        return data[index];
    }

    operator std::conditional_t<std::is_const_v<T>, const T*, T*>() const {
        return data;
    }

    // T** operator&() {
    //     return &data; 
    // }

    using TPtr = T*;
    const GroupArray<T>* refMut() const {
        return this;
    }
    using ConstTPtr = const T*;
    const GroupArray<const T>* refConst() const {
        return (GroupArray<const T>*)this;
    }

    friend struct Group;
};

template<class C>
struct ComponentArray {
private:
    C* data;
public:
    ComponentArray() : data(nullptr) {}

    ComponentArray(C* data) : data(data) {}

    std::conditional_t<std::is_const_v<C>, const C&, C&> operator[](int index) {
        return data[index];
    }
};

struct EntityArray {
private:
    const Entity* data;
public:

    EntityArray() : data(nullptr) {}

    EntityArray(const Entity* data) : data(data) {}

    Entity operator[](int index) const {
        return data[index];
    }
};

struct JobChunk {
    const Job* job;
    void* groupVars;
    const ArchetypePool* pool;
    int indexBegin;
    int indexEnd; // exclusive
    int poolOffset;
};

struct GroupArrayT {
    void** ptrToData;
    size_t capacity; // num elements
    size_t typeSize;
};

struct IComponentGroup {
    Signature read;
    Signature write;
    Signature signature;
    Signature subtract;
};

struct Group {
    const IComponentGroup group;
    using TriggerType = int;
    enum {
        EntityInGroup = 0,
        EntityEntered = 1,
        EntityExited = 2,
        // probably a bad idea, should be done in a different way
        // EntityCreated = 4,
        // EntityDestroyed = 8,
    };
    TriggerType trigger;
    std::vector<GroupArrayT> arrays;
    ArchetypePool* watcherPool = nullptr;

    Group(IComponentGroup group, TriggerType trigger) : group(group), trigger(trigger) {}

    template<typename T>
    void addArray(GroupArray<T>* array) {
        GroupArrayT voidArray;
        voidArray.ptrToData = (void**)&array->data;
        voidArray.typeSize = sizeof(T);
        voidArray.capacity = 0;
        arrays.push_back(voidArray);
    }

    bool operator==(const Group& other) const {
        return group.signature == other.group.signature
            && group.subtract  == other.group.subtract
            && trigger == other.trigger;
    }
};

// // tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
// template<bool TakesEntityArray, class... Components, class... Vars, class JobT>
// void makeJob(JobT* job, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
//     constexpr Signature readSignature = ECS::getSignature<Components...>();
//     constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
//     job->readComponents = readSignature;
//     job->writeComponents = writeSignature;
//     job->dependencies = job; // TODO: remove this field maybeee?
 
//     using GroupVarTuple = std::tuple<Vars*...>; // need pointers to vars so they remain up to date
//     job->executeFunc = [](void* jobDataPtr, int startN, int endN){
//         JobData* jobData = (JobData*)jobDataPtr;
//         JobT* job = (JobT*)jobData->dependencies;
//         GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
//         if constexpr (TakesEntityArray) {
//             EntityArray entityArray = {jobData->getEntityArray()};
//             std::tuple<ComponentArray<Components>...> componentArrays = {
//                 ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
//             };
//             for (int N = startN; N < endN; N++) {
//                 job->Execute(N, entityArray, std::get<TupleTypeIndex<ComponentArray<Components>, ComponentArray<Components>...>>(componentArrays) ..., *std::get<TupleTypeIndex<Vars*, Vars*...>>(groupVars) ...);
//             }
//         } else {
//             std::tuple<ComponentArray<Components>...> componentArrays = {
//                 ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
//             };
//             for (int N = startN; N < endN; N++) {
//                 job->Execute(N, std::get<TupleTypeIndex<ComponentArray<Components>, ComponentArray<Components>...>>(componentArrays) ..., *std::get<TupleTypeIndex<Vars*, Vars*...>>(groupVars) ...);
//             }
//         }
        
//     };
// }

// template<typename GroupVarsT, class ComponentsTuple, class Class>
// Job makeJob4(Class* deps){
//     Job job;
//     //job.readComponents = ECS::getSignature<Components...>();
//     //job.writeComponents = ECS::getMutableSignature<Components...>();
//     // job.parallelizable = parallelizable;
//     job.dependencies = deps;

//     job.executeFunc = [](void* jobDataPtr, int startN, int endN){
//         JobData* jobData = (JobData*)jobDataPtr;
//         Class& deps = *(Class*)jobData->dependencies;
//         auto componentArrays = getComponents(jobData, ComponentsTuple{});
//         // std::tuple<Components*...> componentArrays = {
//         //     jobData->getComponentArray<Components>() ...
//         // };
//         GroupVarsT* groupVars = (GroupVarsT*)jobData->groupVars;
//         for (int N = startN; N < endN; N++) {
//             Class::execute(deps, N, groupVars, componentArrays);
//         }
//     };
//     return job;
// }

// template<class C, typename = void>
// struct HasComponentArrayID : std::false_type {};

// template<class C>
// struct HasComponentArrayID<C, std::void_t<decltype(C::IsComponentArray)>>
//     : std::is_convertible<decltype(C::IsComponentArray), bool> {};

template<typename T>
inline constexpr bool IsComponentArray = is_instantiation_of_v<T, ComponentArray>;

static_assert(IsComponentArray<int> == false, "Is component not detecting invalid components");

template <bool WantComponents, typename... Vars>
struct filter_component_arrays;

template <bool WantComponents>
struct filter_component_arrays<WantComponents> {
    using type = std::tuple<>;
};

template <bool WantComponents, typename Head, typename... Tail>
struct filter_component_arrays<WantComponents, Head, Tail...> {
private:
    using tail_filtered = typename filter_component_arrays<WantComponents, Tail...>::type;
public:
    using type = std::conditional_t<
        IsComponentArray<std::remove_pointer_t<Head>> == WantComponents, // remove pointer so it works pointers too
        decltype(std::tuple_cat(std::declval<std::tuple<Head>>(), std::declval<tail_filtered>())),
        tail_filtered
    >;
};

template <typename... Vars>
struct filter_vars;

template <>
struct filter_vars<> {
    using type = std::tuple<>;
};

template <typename Head, typename... Tail>
struct filter_vars<Head, Tail...> {
private:
    using tail_filtered = typename filter_vars<Tail...>::type;
public:
    using type = std::conditional_t<
        !IsComponentArray<std::remove_const_t<std::remove_pointer_t<Head>>> && !std::is_same_v<std::remove_const_t<std::remove_pointer_t<Head>>, EntityArray>, // remove pointer so it works pointers too
        decltype(std::tuple_cat(std::declval<std::tuple<Head>>(), std::declval<tail_filtered>())),
        tail_filtered
    >;
};

template <typename T>
struct ExecuteMethodTraits;

// Member function pointer
// Separate FirstArg so it isn't classified as a needed argument to the job. It is the index 
template <typename Class, typename Ret, typename FirstArg, typename... Args>
struct ExecuteMethodTraits<Ret(Class::*)(FirstArg, Args...)> {
    using ReturnType = Ret;
    using ClassType = Class;
    using VarsTuple = typename filter_vars<Args...>::type;
    using VarPtrsTuple = typename filter_vars<const Args*...>::type;

    static_assert(std::is_same_v<ReturnType, void>, "Execute method should always return void!");
    static_assert(std::is_same_v<FirstArg, int>, "Execute method must take index as int for first parameter");
};

template <auto Method>
using ExecuteMethodVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template<auto Method>
using JobGroupVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template<auto Method>
using JobGroupVarPtrs = typename ExecuteMethodTraits<decltype(Method)>::VarPtrsTuple;

// tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
template<class JobT, auto Execute, class... Vars>
constexpr JobExePtr makeConditionalExecute() {
    using GroupVarTuple = std::tuple<Vars*...>;
    constexpr JobExePtr executeFunc = [](void* jobPtr, void* groupVarsPtr, int startN, int endN){
        JobT* job = (JobT*)jobPtr;
        GroupVarTuple& groupVars = *(GroupVarTuple*)groupVarsPtr;
        for (int N = startN; N < endN; N++) {
            (job->*Execute)(N, *std::get<TupleTypeIndex<Vars*, Vars*...>>(groupVars) ...);
        }
    };
    return executeFunc;
}

template<class JobT, auto Execute, typename VarsTuple>
struct MakeJobber; 

template<class JobT, auto Execute, typename... Vars>
struct MakeJobber<JobT, Execute, std::tuple<Vars...>> {
    static constexpr JobExePtr makeExecuteFunc() {
        // need pointers to vars so they remain up to date
        constexpr JobExePtr executeFunc = [](void* jobPtr, void* groupVarsPtr, int startN, int endN){
            JobT* job = (JobT*)jobPtr;
            auto& groupVars = *(std::tuple<Vars*...>*)groupVarsPtr;
            for (int N = startN; N < endN; N++) {
                if constexpr (Execute == &JobT::Execute) {
                    job->Execute(N, *std::get<TupleTypeIndex<Vars*, Vars*...>>(groupVars) ...);
                } else {
                    (job->*Execute)(N, *std::get<TupleTypeIndex<Vars*, Vars*...>>(groupVars) ...);
                }
            }
        };
        return executeFunc;
    }
};

template<typename Derived, typename... Components>
struct JobDecl : Job {
    static_assert(sizeof...(Components) <= 8, "Jobs must use a maximum of 8 component arrays!");
    // because of Job::componentIDs array being 8 elements

    template<typename Component, typename... Cs>
    void setComponentID(int index) {
        this->componentIDs[index] = Component::ID;
    }
    
    JobDecl() {
        static_assert(std::is_trivially_copyable_v<Derived>, "Jobs must be trivially copyable!");

        static constexpr Signature readSignature = ECS::getSignature<Components...>();
        static constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
        this->readComponents = readSignature;
        this->writeComponents = writeSignature;

        static constexpr auto componentIDs = ECS::getComponentIDs<Components...>();

        for (int i = 0; i < sizeof...(Components); i++) {
            this->componentIDs[i] = (Uint8)componentIDs[i];
        }
        this->executeFunc = MakeJobber<Derived, &Derived::Execute, JobGroupVars<&Derived::Execute>>::makeExecuteFunc();

        size = sizeof(Derived);
    }

    template<class C>
    using ComponentReturnType = std::conditional_t<
        (std::disjunction_v<std::is_same<C, Components>...>),
        C&,
        std::conditional_t<
            (std::disjunction_v<std::is_same<const C, Components>...>),
            C,
            C& // fallback
        >
    >;


    template<class Component>
    [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE ComponentReturnType<Component>
    Get(int N) const {
        static_assert(is_one_of_v<std::remove_const_t<Component>, std::remove_const_t<Components>...>, "Tried to Get component not declared in job template, add it if you forgot!");
        static constexpr int index = TupleTypeIndex<std::remove_const_t<Component>, std::remove_const_t<Components>...>;
        return static_cast<Component*>(this->componentArrays[index])[N];
    }

    [[nodiscard]] LLVM_ATTRIBUTE_ALWAYS_INLINE Entity& GetEntity(int N) const {
        return entities[N];
    }

    template<auto Exe, class FirstComponentNeeded, class... RestComponentsNeeded>
    void addConditionalExecute() {
        if (nConditionalExecutions == MaxConditionalExecutions) {
            LogError("Reached limit of number of conditional job executions! skipping...");
            return;
        }

        static constexpr Signature readSignature = ECS::getSignature<FirstComponentNeeded, RestComponentsNeeded...>();
        static constexpr Signature writeSignature = ECS::getMutableSignature<FirstComponentNeeded, RestComponentsNeeded...>();
        this->readComponents |= readSignature;
        this->writeComponents |= writeSignature;
        conditionalExecutions[nConditionalExecutions++] = {
            readSignature,
            MakeJobber<Derived, Exe, JobGroupVars<&Derived::Execute>>::makeExecuteFunc()
        };

    }
};

// the arguments required to a job
template<typename Job>
using JobArgs = JobGroupVars<&Job::Execute>;

template<typename Job>
using JobArgPtrs = JobGroupVarPtrs<&Job::Execute>;

} // end namespace System

} // end namespace ECS