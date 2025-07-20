#pragma once

#include "utils/type_traits.hpp"
#include "ECS/Signature.hpp"
#include "ECS/EntityManager.hpp"
#include <vector>

namespace ECS {

namespace Systems {

using JobExePtr = void(*)(void*, int startIndex, int endIndex);

struct Job {
    JobExePtr executeFunc = nullptr;
    Signature readComponents = 0;
    Signature writeComponents = 0;
    void* dependencies = nullptr;
    bool parallelize     : 1;
    bool mainThread      : 1;
    bool blocking        : 1;
    bool enabled         : 1;
    

    struct ConditionalExecution {
        ECS::Signature required;
        JobExePtr execute;
    };
    std::vector<ConditionalExecution> conditionalExecutions;

    Job() {
        parallelize = true;
        mainThread = false;
        blocking = false;
        enabled = true;
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
    std::vector<Entity> triggeredEntities;

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

// template<typename T>
// constexpr GroupArray<T>::GroupArray(Group* group) {
//     group->addArray(this);
// }

// tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
template<bool TakesEntityArray, class... Components, class... Vars, class JobT>
void makeJob(JobT* job, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    constexpr Signature readSignature = ECS::getSignature<Components...>();
    constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
    job->readComponents = readSignature;
    job->writeComponents = writeSignature;
    job->dependencies = job; // TODO: remove this field maybeee?
 
    using GroupVarTuple = std::tuple<Vars*...>; // need pointers to vars so they remain up to date
    job->executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobData* jobData = (JobData*)jobDataPtr;
        void* job = jobData->dependencies;
        GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
        if constexpr (TakesEntityArray) {
            std::tuple<EntityArray, ComponentArray<Components>...> componentArrays = {
                EntityArray(jobData->getEntityArray()),
                ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
            };
            for (int N = startN; N < endN; N++) {
                JobT::execute(job, N, componentArrays, groupVars);
            }
        } else {
            std::tuple<ComponentArray<Components>...> componentArrays = {
                ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
            };
            for (int N = startN; N < endN; N++) {
                JobT::execute(job, N, componentArrays, groupVars);
            }
        }
        
    };
}

// template<class ...Components>
// std::tuple<Components*...> getComponents(JobData* jobData, std::tuple<Components...> dummy) {
//     return { jobData->getComponentArray<Components>() ... };
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
    using ComponentArraysTuple = typename filter_component_arrays<true, Args...>::type;
    using VarsTuple = typename filter_vars<Args...>::type;
    using VarPtrsTuple = typename filter_vars<const Args*...>::type;

    template<typename T, typename = void>
    struct takes_entity_array : std::false_type {};

    template<typename T>
    struct takes_entity_array<T, std::enable_if_t<(std::tuple_size_v<T> > 0)>>
    : std::is_same<std::tuple_element_t<0, T>, EntityArray> {};

    static constexpr bool TakesEntityArray = takes_entity_array<std::tuple<Args...>>::value;

    static_assert(std::is_same_v<ReturnType, void>, "Execute method should always return void!");
    static_assert(std::is_same_v<FirstArg, int>, "Execute method must take index as int for first parameter");
};

template <auto Method>
using ExecuteMethodVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template <auto Method>
using ExecuteMethodComponents = typename ExecuteMethodTraits<decltype(Method)>::ComponentArraysTuple;

template<auto Method>
using JobGroupVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template<auto Method>
using JobGroupVarPtrs = typename ExecuteMethodTraits<decltype(Method)>::VarPtrsTuple;

template<typename JobT>
constexpr bool JobTakesEntityArray = ExecuteMethodTraits<decltype(&JobT::Execute)>::TakesEntityArray;

// tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
template<bool TakesEntityArray, class Execute, class... Components, class... Vars>
Job::ConditionalExecution makeConditionalExecute(Job* job, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    constexpr Signature readSignature = ECS::getSignature<Components...>();
    constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
    job->readComponents |= readSignature;
    job->writeComponents |= writeSignature;
 
    using GroupVarTuple = std::tuple<const Vars*...>;
    JobExePtr executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobData* jobData = (JobData*)jobDataPtr;
        Job* job = (Job*)jobData->dependencies;
        GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
        if constexpr (TakesEntityArray) {
            std::tuple<EntityArray, ComponentArray<Components>...> componentArrays = {
                EntityArray(jobData->getEntityArray()),
                ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
            };
            for (int N = startN; N < endN; N++) {
                Execute::execute(job, N, componentArrays, groupVars);
            }
        } else {
            std::tuple<ComponentArray<Components>...> componentArrays = {
                ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
            };
            for (int N = startN; N < endN; N++) {
                Execute::execute(job, N, componentArrays, groupVars);
            }
        }
    };
    return {
        readSignature,
        executeFunc
    };
}

template<typename Derived>
struct JobDecl : Job {
    JobDecl() {
        makeJob<JobTakesEntityArray<Derived>>(
            static_cast<Derived*>(this), 
            (ExecuteMethodComponents<&Derived::Execute>*)nullptr, 
            (ExecuteMethodVars<&Derived::Execute>*)nullptr);
    }

    // or entities
    template<typename... Ts, typename... GroupVars>
    static constexpr void execute(void* self, int N, std::tuple<Ts...> components, std::tuple<GroupVars*...> vars) {
        ((Derived*)self)->Execute(N, std::get<TupleTypeIndex<Ts, Ts...>>(components) ..., *std::get<TupleTypeIndex<GroupVars*, GroupVars*...>>(vars) ...);
    }

    template<auto Exe>
    struct Execute {
        template<typename... Components, typename... GroupVars>
        static constexpr void execute(void* self, int N, std::tuple<ComponentArray<Components>...> components, std::tuple<GroupVars*...> vars) {
            (((Derived*)self)->*Exe)(N, std::get<TupleTypeIndex<Components*, Components*...>>(components) ..., *std::get<TupleTypeIndex<GroupVars*, GroupVars*...>>(vars) ...);
        }
    };

    template<auto Exe>
    void addConditionalExecute() {
        conditionalExecutions.push_back(
            makeConditionalExecute<ExecuteMethodTraits<decltype(Exe)>::TakesEntityArray, Execute<Exe>>(
                this, 
                (ExecuteMethodComponents<Exe>*)nullptr, 
                (ExecuteMethodVars<&Derived::Execute>*)nullptr // Vars to both executes must be the same
            )
        );
    }
};

// the arguments required to a job
template<typename Job>
using JobArgs = JobGroupVars<&Job::Execute>;

template<typename Job>
using JobArgPtrs = JobGroupVarPtrs<&Job::Execute>;

} // end namespace System

} // end namespace ECS