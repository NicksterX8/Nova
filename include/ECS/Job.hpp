#pragma once

#include "utils/map.h"
#include "utils/common-macros.hpp"
#include "ECS/system.hpp"
#include "utils/type_traits.hpp"

namespace ECS {

namespace System {

namespace New {

struct Job {
    JobExePtr executeFunc = nullptr;
    Signature readComponents = 0;
    Signature writeComponents = 0;
    void* dependencies = nullptr;
    bool parallelizable = false;
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

    // void addConditionalExecute(ECS::Signature requiredSignature, JobExePtr exeFunc) {
    //     conditionalExecutions.push_back({requiredSignature, exeFunc});
    // }

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

    void* groupArrays;

    ArchetypePool* pool;
    int indexBegin;

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
};

struct GroupArrayI {};

struct Group;

template<typename T>
struct GroupArray : GroupArrayI {
private:
    T* data = nullptr;
public:
    // constexpr GroupArray() {}

    constexpr GroupArray(Group* group);

    T& operator[](int index){
        return data[index];
    }

    const T& operator[](int index) const {
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
    using TriggerType = int;
    enum {
        EntityEnters = 1,
        EntityExits = 2,
        EntityCreated = 4,
        EntityDestroyed = 8,
        EntityInGroup = 16
    };
    TriggerType trigger;

    std::vector<GroupArrayT> arrays;

    Group(IComponentGroup group, TriggerType trigger) : group(group), trigger(trigger) {}

    template<typename T>
    void addArray(GroupArray<T>* array) {
        GroupArrayT voidArray;
        voidArray.ptrToData = (void**)&array->data;
        voidArray.typeSize = sizeof(T);
        voidArray.capacity = 0;
        arrays.push_back(voidArray);
    }
};

template<typename T>
constexpr GroupArray<T>::GroupArray(Group* group) {
    group->addArray(this);
}

// tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
template<class... Components, class... Vars, class JobT>
void makeJob(JobT* job, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    constexpr Signature readSignature = ECS::getSignature<Components...>();
    constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
    job->readComponents = readSignature;
    job->writeComponents = writeSignature;
    // job.parallelizable = parallelizable;
    job->dependencies = job;
 
    using GroupVarTuple = std::tuple<Vars...>; // don't use the first var, because it is the index N
    job->executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        void* job = jobData->dependencies;
        std::tuple<ComponentArray<Components>...> componentArrays = {
            ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
        };
        GroupVarTuple& groupVars = *(GroupVarTuple*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            JobT::execute(job, N, componentArrays, groupVars);
        }
    };
}

// template<class ...Components>
// std::tuple<Components*...> getComponents(JobDataNew* jobData, std::tuple<Components...> dummy) {
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
//         JobDataNew* jobData = (JobDataNew*)jobDataPtr;
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

template<class C, typename = void>
struct HasComponentArrayID : std::false_type {};

template<class C>
struct HasComponentArrayID<C, std::void_t<decltype(C::IsComponentArray)>>
    : std::is_convertible<decltype(C::IsComponentArray), bool> {};

template<typename T>
inline constexpr bool IsComponentArray = HasComponentArrayID<T>::value;

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
        IsComponentArray<Head> == WantComponents,
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
    using VarsTuple = typename filter_component_arrays<false, Args...>::type;

    static_assert(std::is_same_v<ReturnType, void>, "Execute method should always return void!");
    static_assert(std::is_same_v<FirstArg, int>, "Execute method must take index as int for first parameter");
};

template <auto Method>
using ExecuteMethodVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template <auto Method>
using ExecuteMethodComponents = typename ExecuteMethodTraits<decltype(Method)>::ComponentArraysTuple;

template<auto Method>
using JobGroupVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

// tuple parameters are used only to take in variadic type packs. Value does not matter including nullptr
template<class Execute, class... Components, class... Vars>
Job::ConditionalExecution makeConditionalExecute(Job* job, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    constexpr Signature readSignature = ECS::getSignature<Components...>();
    constexpr Signature writeSignature = ECS::getMutableSignature<Components...>();
    job->readComponents |= readSignature;
    job->writeComponents |= writeSignature;
 
    using GroupVarTuple = std::tuple<Vars...>; // don't use the first var, because it is the index N
    JobExePtr executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Job* job = (Job*)jobData->dependencies;
        std::tuple<ComponentArray<Components>...> componentArrays = {
            ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
        };
        GroupVarTuple& groupVars = *(GroupVarTuple*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            Execute::execute(job, N, componentArrays, groupVars);
        }
    };
    return {
        readSignature,
        executeFunc
    };
}

template<typename Derived>
struct JobDeclNew : Job {

    JobDeclNew() {
        makeJob(
            static_cast<Derived*>(this), 
            (ExecuteMethodComponents<&Derived::Execute>*)nullptr, 
            (ExecuteMethodVars<&Derived::Execute>*)nullptr);
    }

    template<typename... Components, typename... GroupVars>
    static constexpr void execute(void* self, int N, std::tuple<ComponentArray<Components>...> components, std::tuple<GroupVars...> vars) {
        ((Derived*)self)->Execute(N, std::get<TupleTypeIndex<Components*, Components*...>>(components) ..., std::get<TupleTypeIndex<GroupVars, GroupVars...>>(vars) ...);
    }

    template<auto Exe>
    struct Execute {
        template<typename... Components, typename... GroupVars>
        static constexpr void execute(void* self, int N, std::tuple<ComponentArray<Components>...> components, std::tuple<GroupVars...> vars) {
            (((Derived*)self)->*Exe)(N, std::get<TupleTypeIndex<Components*, Components*...>>(components) ..., std::get<TupleTypeIndex<GroupVars, GroupVars...>>(vars) ...);
        }
    };

    template<auto Exe>
    void addConditionalExecute() {
        conditionalExecutions.push_back(
            makeConditionalExecute<Execute<Exe>>(
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


} // end namespace New

} // end namespace System

} // end namespace ECS