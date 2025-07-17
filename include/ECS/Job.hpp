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

    void* groupArrays;

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

template <typename T, typename... Ts>
struct Index;

template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};

template <typename T, typename... Ts>
constexpr std::size_t Index_v = Index<T, Ts...>::value;

template<class... Components, class... Vars, class Class>
Job makeJob(Class* deps, std::tuple<ComponentArray<Components>...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    Job job;
    job.readComponents = ECS::getSignature<Components...>();
    job.writeComponents = ECS::getMutableSignature<Components...>();
    // job.parallelizable = parallelizable;
    job.dependencies = deps;
 
    using GroupVarTuple = std::tuple<Vars...>; // don't use the first var, because it is the index N
    job.executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Class& deps = *(Class*)jobData->dependencies;
        std::tuple<ComponentArray<Components>...> componentArrays = {
            ComponentArray<Components>(jobData->getComponentArray<Components>()) ...
        };
        GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            Class::execute(deps, N, componentArrays, groupVars);
        }
    };
    return job;
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

// For free functions or static functions
template <typename T>
struct ExecuteMethodTraits;

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

// Member function pointer
template <typename Class, typename Ret, typename FirstArg, typename... Args>
struct ExecuteMethodTraits<Ret(Class::*)(FirstArg, Args...)> {
    using ReturnType = Ret;
    using ClassType = Class;
    using ArgsTuple = std::tuple<Args...>;
    using ComponentArraysTuple = typename filter_component_arrays<true, Args...>::type;
    using VarsTuple = typename filter_component_arrays<false, Args...>::type;
    // using GroupVarsT = std::remove_pointer_t<std::tuple_element_t<2, ArgsTuple>>;
    //using ComponentTuple = std::tuple<std::tuple_element_t<1, std::remove_pointer<ArgsTuple>::type>>;

    static_assert(std::is_same_v<ReturnType, void>, "Execute method should always return void!");
    static_assert(std::is_same_v<FirstArg, int>, "Execute method must take index as int for first parameter");
};

template <auto Method>
using ExecuteMethodVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template <auto Method>
using ExecuteMethodComponents = typename ExecuteMethodTraits<decltype(Method)>::ComponentArraysTuple;

template<auto Method>
using JobGroupVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template<typename Derived>
struct JobDeclNew : Job {

    JobDeclNew() : Job(makeJob(
        static_cast<Derived*>(this), 
        (ExecuteMethodComponents<&Derived::Execute>*)nullptr, 
        (ExecuteMethodVars<&Derived::Execute>*)nullptr)) {}

    template<typename... Components, typename... GroupVars>
    static constexpr void execute(Derived& self, int N, std::tuple<ComponentArray<Components>...> components, std::tuple<GroupVars...> vars) {
        self.Execute(N, std::get<Index_v<Components*, Components*...>>(components) ..., std::get<Index_v<GroupVars, GroupVars...>>(vars) ...);
    }
};

template<typename Job>
using JobArgs = JobGroupVars<&Job::Execute>;

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
        Signature illegalReads = job.readComponents & ~group->group.read;
        if (illegalReads.any()) {
            LogError("Illegal job component reads!");
        }
        Signature illegalWrites = job.writeComponents & ~group->group.write;
        if (illegalWrites.any()) {
            LogError("Illegal job component writes!");
        }

        void* argsPtr = new JobArgs<JobT>(args);

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
};


} // end namespace New

} // end namespace System

} // end namespace ECS