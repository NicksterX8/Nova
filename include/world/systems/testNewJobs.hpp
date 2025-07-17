#pragma once

#include "ECS/Job.hpp"

#include "GUI/components.hpp"
#include "world/components/components.hpp"
#include "ECS/TriggerSystem.hpp"

#include "utils/type_traits.hpp"
#include <string>

#include <tuple>


using namespace ECS::System;
using namespace ECS::System::New;

struct ChunkMap;

struct GroupVars {};


namespace GUI {

struct PositionsArray {
    GroupArray<int> positions;
};

struct MoveJob2 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    MoveJob2(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        USE_GROUP_VARS(PositionsArray);
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
        SetConst<EC::ViewBox>();
    }

    void update(int scale) {
        this->scale = scale;
    }
};

struct MoveJob3 : Job {
    int scale = -1;
    ChunkMap* chunkmap;

    USE_GROUP_VARS(PositionsArray);

    MoveJob3(ChunkMap* chunkmap) : chunkmap(chunkmap) {
        EXECUTE_START_CLASS(ViewBox)
            groupVars.positions[N] = ViewBox[N].box.min.x * self.scale;
        EXECUTE_END_CLASS
    }

    void update(int scale) {
        this->scale = scale;
    }
};

struct NewSystemEx : NewSystem {
    ChunkMap* chunkmap;
    MoveJob2 moveJob;

    Group* group = makeGroup(
        ComponentGroup<
            ReadOnly<EC::ViewBox>
        >(), &positions);

    PositionsArray positions{group};

    NewSystemEx(SystemManager& manager, ChunkMap* chunkmap) : NewSystem(manager),
    chunkmap(chunkmap), moveJob(chunkmap) {
        // Group* group = makeGroup(ComponentGroup<
        //     ReadOnly<EC::ViewBox>
        // >(), &positions);
        // group->addArray(&positions.positions);
        
        Schedule(group, moveJob);
    }

    void Update(int scale) {
        moveJob.update(scale);
    }
};

template <typename T, typename... Ts>
struct Index;

template <typename T, typename... Ts>
struct Index<T, T, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename T, typename U, typename... Ts>
struct Index<T, U, Ts...> : std::integral_constant<std::size_t, 1 + Index<T, Ts...>::value> {};

template <typename T, typename... Ts>
constexpr std::size_t Index_v = Index<T, Ts...>::value;

template<class Execute, class... Components, class Deps, class... T>
Job makeJob(Deps* deps, GroupArray<T>*... groupArrays) {
    Job job;
    job.readComponents = ECS::getSignature<Components...>();
    job.writeComponents = ECS::getMutableSignature<Components...>();
    // job.parallelizable = parallelizable;
    job.dependencies = deps;
    // job.dependencies.groupArrays.push_back(groupArray);
    using GroupArrayTuple = std::tuple<GroupArray<T>*...>;
    job.groupArrays = new GroupArrayTuple{groupArrays ...};
    job.executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Deps& deps = *(Deps*)jobData->dependencies;
        std::tuple<Components*...> componentArrays = {
            jobData->getComponentArray<Components>() ...
        };
        GroupArrayTuple groupArrays = *(GroupArrayTuple*)jobData->groupArrays;
        for (int N = startN; N < endN; N++) {
            Execute::execute(deps, N, std::get<Components*>(componentArrays) ..., *std::get<Index<T, T...>::value>(groupArrays) ...);
        }
    };
    return job;
}

template<typename... Ts>
struct GroupVarsList {};

template<class Execute, class... Components, class... Vars, class Deps>
Job makeJob2(Deps* deps, GroupVarsList<Vars...>) {
    Job job;
    job.readComponents = ECS::getSignature<Components...>();
    job.writeComponents = ECS::getMutableSignature<Components...>();
    // job.parallelizable = parallelizable;
    job.dependencies = deps;
    // job.dependencies.groupArrays.push_back(groupArray);
    using GroupVarTuple = std::tuple<Vars*...>;
    // job.groupArrays = new GroupArrayTuple{groupArrays ...};
    job.executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Deps& deps = *(Deps*)jobData->dependencies;
        std::tuple<Components*...> componentArrays = {
            jobData->getComponentArray<Components>() ...
        };
        GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            Execute::execute(N, deps, std::get<Components*>(componentArrays) ..., *std::get<Index<Vars*, Vars*...>::value>(groupVars) ...);
        }
    };
    return job;
}

template<class... Components, class... Vars, class Class>
Job makeJob3(Class* deps, std::tuple<Components...>* /*nullptr*/, std::tuple<Vars...>* /*nullptr*/) {
    Job job;
    job.readComponents = ECS::getSignature<Components...>();
    job.writeComponents = ECS::getMutableSignature<Components...>();
    // job.parallelizable = parallelizable;
    job.dependencies = deps;

    using GroupVarTuple = TupleMinusFirst<Vars&...>; // don't use the first var, because it is the index N
    job.executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Class& deps = *(Class*)jobData->dependencies;
        std::tuple<Components*...> componentArrays = {
            jobData->getComponentArray<Components>() ...
        };
        GroupVarTuple groupVars = *(GroupVarTuple*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            Class::execute(deps, N, componentArrays, groupVars);
        }
    };
    return job;
}

template<class ...Components>
std::tuple<Components*...> getComponents(JobDataNew* jobData, std::tuple<Components...> dummy) {
    return { jobData->getComponentArray<Components>() ... };
}

template<typename GroupVarsT, class ComponentsTuple, class Class>
Job makeJob4(Class* deps){
    Job job;
    //job.readComponents = ECS::getSignature<Components...>();
    //job.writeComponents = ECS::getMutableSignature<Components...>();
    // job.parallelizable = parallelizable;
    job.dependencies = deps;

    job.executeFunc = [](void* jobDataPtr, int startN, int endN){
        JobDataNew* jobData = (JobDataNew*)jobDataPtr;
        Class& deps = *(Class*)jobData->dependencies;
        auto componentArrays = getComponents(jobData, ComponentsTuple{});
        // std::tuple<Components*...> componentArrays = {
        //     jobData->getComponentArray<Components>() ...
        // };
        GroupVarsT* groupVars = (GroupVarsT*)jobData->groupVars;
        for (int N = startN; N < endN; N++) {
            Class::execute(deps, N, groupVars, componentArrays);
        }
    };
    return job;
}

// For free functions or static functions
template <typename T>
struct ExecuteMethodTraits;

// Function pointer
template <typename Ret, typename... Args>
struct ExecuteMethodTraits<Ret(*)(Args...)> {
    using ReturnType = Ret;
    using ArgsTuple = std::tuple<Args...>;
};

// Member function pointer
template <typename Class, typename Ret, typename... Args>
struct ExecuteMethodTraits<Ret(Class::*)(Args...)> {
    using ReturnType = Ret;
    using ClassType = Class;
    using ArgsTuple = std::tuple<Args...>;
    //using ComponentsTuple = ECS::filter_components_t<true, std::remove_pointer_t<Args>...>;
    using ComponentsTuple = std::tuple<std::remove_pointer_t<std::tuple_element_t<1, ArgsTuple>>>;
    using VarsTuple = ECS::filter_components_t<false, Args...>;
    using GroupVarsT = std::remove_pointer_t<std::tuple_element_t<2, ArgsTuple>>;
    //using ComponentTuple = std::tuple<std::tuple_element_t<1, std::remove_pointer<ArgsTuple>::type>>;

    static_assert(std::is_same_v<ReturnType, void>, "Execute method should always return void!");
    static_assert(std::is_same_v<std::tuple_element_t<0, ArgsTuple>, int>, "Execute method must take index as int for first parameter");
};

template <auto Method>
using ExecuteMethodVars = typename ExecuteMethodTraits<decltype(Method)>::VarsTuple;

template <auto Method>
using ExecuteMethodComponents = typename ExecuteMethodTraits<decltype(Method)>::ComponentsTuple;

template<auto Method>
using JobGroupVars = typename ExecuteMethodTraits<decltype(Method)>::GroupVarsT;

template<typename Derived>
struct JobDecl2 : Job {
    JobDecl2() : Job(makeJob4<JobGroupVars<&Derived::Execute>, ExecuteMethodComponents<&Derived::Execute>>(static_cast<Derived*>(this))) {

    }

    template<typename... Components, typename GroupVars>
    static constexpr void execute(Derived& self, int N, GroupVars* vars, std::tuple<Components*...> components) {
        self.Execute(N, std::get<Index_v<Components*, Components*...>>(components) ..., vars);
    }

    template<typename... Components>
    static constexpr void execute(Derived& self, int N, std::tuple<Components*...> components) {
        self.Execute(N, std::get<Index_v<Components*, Components*...>>(components) ...);
    }
};

struct EnterGroupVars {
    GroupArray<int> positionsOut;
};

struct MoveJob4 : JobDecl2<MoveJob4> {
    int counter = 0;

    void Execute(int N, EC::ViewBox* viewbox, EnterGroupVars* positionsOut) {
        positionsOut->positionsOut[N] = viewbox[N].box.min.x + counter++;
    }
};

struct NewEntityViewBoxSystem : NewSystem {
    Group* enterGroup = makeGroup(ComponentGroup<
        ReadOnly<EC::ViewBox>
    >(), Group::EntityEnters);

    EnterGroupVars enterGroupVars{enterGroup};
    static_assert(sizeof(ExecuteMethodComponents<&MoveJob4::Execute>) > 1, "is componetn working");

    MoveJob2 moveJob;

    MoveJob4 moveJob4;

    int scale = 10;

    NewEntityViewBoxSystem(SystemManager& manager, ChunkMap* chunkmap) 
    : NewSystem(manager), moveJob(chunkmap) {
        // struct Func {
        //     static constexpr void execute(int N, int& scale, EC::ViewBox* viewboxes, GroupArray<int> output, GroupArray<int> scales, float scale2) {
        //         EC::ViewBox viewbox = viewboxes[N];
        //         viewbox.box.min.x *= scale;
        //         output[N] = viewbox.box.max().y * scale + 2;
        //     }
        // };
        Schedule(enterGroup, moveJob4);

        // Group* exitGroup = makeGroup(ComponentGroup<
        //     ReadOnly<EC::ViewBox>
        // >(), Group::EntityExits);
        // Schedule(exitGroup, moveJob);
    }
};


}