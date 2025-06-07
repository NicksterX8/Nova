#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>
#include "ecs.hpp"
#include "My/Vec.hpp"
#include "ECS/Entity.hpp"

namespace EcsSystem {

using Element = ECS::Entity;
using ElementCommandBuffer = GECS::ElementCommandBuffer;

struct AbstractComponentGroup {
    GECS::EcsClass::Signature read;
    GECS::EcsClass::Signature write;
    GECS::EcsClass::Signature signature;
    GECS::EcsClass::Signature subtract;
};

struct IJob;

struct Task {
    AbstractComponentGroup group;
    IJob* job;
};

struct ISystem;

struct TaskGroup {
    AbstractComponentGroup group;
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
    My::Vec<char>* memory = nullptr;
    int typeSize = -1;
    // for element array
    bool elementArray = false;
};

struct IJob {
    // metadata stuff all jobs might need
    // int time;
    ElementCommandBuffer commands;

    std::vector<AbstractGroupArray*> arrays;

    enum Type {
        Parallel,
        MainThread
    } type;

    IJob(Type type) : type(type) {

    }

    virtual void Execute(int N) = 0;
};

struct SystemManager {
    std::vector<ISystem*> systems;  
};

struct ScheduledJob {
    AbstractComponentGroup* group;
    IJob* job;
    int order;
};

struct Scheduler {
    ISystem* system;
    IJob* job;

    Scheduler& DependentOn(IJob& jobs) {
        return *this;
    }
};

struct ISystem {
    bool enabled;
    int order;

    ElementCommandBuffer commands;

    // job schedule
    std::vector<ScheduledJob> schedule;

    ISystem(SystemManager& manager, int order = 0) : enabled(true), order(order) {
        manager.systems.push_back(this);
    }

    virtual void ScheduleJobs() = 0;

    virtual void BeforeExecution() {}

    Scheduler Schedule(AbstractComponentGroup& group, IJob& job) {
        schedule.push_back({&group, &job, 0});
        return Scheduler{.system = this, .job = &job};
    }

    void AddDependency(IJob& dependent, IJob* dependency) {

    }

    virtual ~ISystem() {

    }
};

template<typename T>
struct GroupArray : AbstractGroupArray {
    GroupArray(const GroupArray<T>& other) = delete;

    GroupArray(IJob* job, const GroupArray<std::remove_const_t<T>>& other) {
        this->data = other.data;
        this->componentType = other.componentType;
        this->memory = other.memory;
        this->typeSize = other.typeSize;
        this->readonly = std::is_const_v<T>;
        this->elementArray = false;

        if (job)
            job->arrays.push_back(this);
    }

protected:
    GroupArray(IJob* job) {
        if (job)
            job->arrays.push_back(this);
    }
public:

    typename std::conditional<std::is_const_v<T>, const T&, T&>::type& operator[](int index) {
        return ((T*)data)[index];
    }
};

template<class C>
struct ComponentArray : GroupArray<C> {
    ComponentArray(IJob* job) : GroupArray<C>(job) {
        this->componentType = C::ID;
        this->readonly = std::is_const_v<C>;
    }
};

template<typename T>
struct PlainArray : GroupArray<T> {

    //PlainArray(IJob* job, const PlainArray<T>& other) : GroupArray<T>(job, other) {}
    PlainArray(IJob* job, const PlainArray<std::remove_const_t<T>>& other) : GroupArray<T>(job, other) {}

    // owned by job
    PlainArray(IJob* job) : GroupArray<T>(job) {
        this->memory = new My::Vec<char>(0);
        this->typeSize = sizeof(T);
        this->readonly = std::is_const_v<T>;
    }
};

struct ElementArray : AbstractGroupArray {
    ElementArray(IJob* job) {
        elementArray = true;
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
struct ComponentGroup : AbstractComponentGroup {
    template<class C>
    void setComponentUses() {
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

    ComponentGroup() {
        int dummy[] = {0, (setComponentUses<Cs>(), 0) ...};
        (void)dummy;
    }
};

void executeSystems(SystemManager&, GECS::ElementManager&);

}


#endif