#ifndef ECS_GENERIC_SYSTEM_INCLUDED
#define ECS_GENERIC_SYSTEM_INCLUDED

#include <vector>

struct PositionComponent {
    int x;
    int y;
};

struct MotionComponent {
    int vx;
    int vy;
};

class AbstractComponentArray {
    
};

struct AbstractComponentGroup {
    std::vector<void*> arrays;
    int size;
    int requiredSignature;
    int excludedSignature;
};

struct Job;

struct JobManager {
    std::vector<Job*> jobs;
    int signature;
    std::vector<void*> componentArrays;
    std::vector<void*> normalArrays;
    AbstractComponentGroup* componentGroup;

    JobManager() {}

    template<class C>
    C* getComponents() {
        if (signature & C::ID) {
            return (C*)componentArrays[C::ID];
        }
        return nullptr;// eerror
    }

    template<class T>
    T* makeArray() {
        return (T*)malloc(componentGroup->size * sizeof(T));
    }
};

template<class C>
class ComponentArray : AbstractComponentArray {
    C* components;
public:
    ComponentArray(JobManager& manager) : AbstractComponentArray() {
        components = manager.getComponents<C>();
    }

    C& operator[](int index) {
        return components[index];
    }

    const C& operator[](int index) const {
        return components[index];
    }
};

struct Job {
    // metadata stuff all jobs might need
    int time;

    Job(JobManager& manager) {
        manager.jobs.push_back(this);
    }

    virtual void execute(int N);
};

struct System;

struct ArchetypeManager {

};

struct SystemManager {
    std::vector<System*> systems;  


};

struct Permissions {
    int read; // signature
    int write;
};



struct System {
    Permissions permssions;

    JobManager jobManager;
    System(SystemManager& manager) {
        manager.systems.push_back(this);
    }

    virtual void instantiateJobs();
    virtual void scheduleJobs();
};

template<typename T>
class PlainArray {
    T* data;
public:
    PlainArray() : data(nullptr) {}

    PlainArray(JobManager& manager) {
        data = manager.makeArray<T>();
    }

    T& operator[](int index) {
        return data[index];
    }
    const T& operator[](int index) const {
        return data[index];
    }
};


template<class ...Cs>
struct ComponentGroup : AbstractComponentGroup {
    ComponentGroup(SystemManager& manager) {
        
    }
};


struct SystemRunner {
    SystemManager& systemManager;

    void runSystem() {

    }
};



// custom jobs and systems


// job is created once per frame.
struct MotionJob : Job {

    ComponentArray<PositionComponent> position;
    ComponentArray<MotionComponent> motion;

    PlainArray<int> newPositions; // intermediate data

    float dt;

    // instantiate component arrays
    MotionJob(JobManager& manager, float deltaTime, PlainArray<int> newPositions)
    : Job(manager), position(manager), motion(manager), dt(deltaTime), newPositions(newPositions) {
        
    }

    void execute(int N) {
        newPositions[N] = position[N].x + motion[N].vx;
    }
};

struct Task {
    AbstractComponentGroup* group;
    Job* job;
};

std::vector<Task> tasks;

template<class... Cs>
void scheduleJob(ComponentGroup<Cs...>* group, Job* job) {
    tasks.push_back({group, job});
}

void instantiateSystems(SystemManager&);

void executeSystems() {
    static SystemManager sysManager;
    instantiateSystems(sysManager);
    
    for (System* system : sysManager.systems) {
        system->instantiateJobs();
        system->scheduleJobs();
    }

    for (Task& task : tasks) {
        for (int i = 0; i < task.group->size; i++) {
            task.job->execute(i);
        }
    }
}

struct MotionSystem : System {
    ComponentGroup<PositionComponent, MotionComponent> group;

    PlainArray<int> newPositions;

    //static constexpr Permissions permissions = systemPermissions();

    MotionJob* motionJob;

    float deltaTime;

    MotionSystem(SystemManager& manager, float deltaTime)
    : System(manager), group(manager), newPositions(jobManager), deltaTime(deltaTime) {

    }

    void instantiateJobs() {
        *motionJob = MotionJob(jobManager, deltaTime, newPositions);
    }

    void scheduleJobs() {
        scheduleJob(&group, motionJob);
    }
};

void instantiateSystems(SystemManager& manager) {
    MotionSystem motionSys(manager, 0.5f);
}

#endif