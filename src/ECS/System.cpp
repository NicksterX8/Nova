#include "ECS/System.hpp"
#include "ADT/SmallVector.hpp"
#include "llvm/TinyPtrVector.h"
#include "global.hpp"
#include "moodycamel/concurrentqueue.h"
#include "ADT/concurrent/AtomicCountdown.hpp"
#include "memory/allocators.hpp"
#include "utils/system/sysinfo.hpp"
#include "memory/StackAllocate.hpp"

using namespace ECS;
using namespace ECS::Systems;

int ECS::Systems::findEligiblePools(Signature required, Signature rejected, const ECS::EntityManager& entityManager, std::vector<const ArchetypePool*>* eligiblePools) {
    int eligibleEntities = 0;
    for (int p = 0; p < entityManager.components.pools.size(); p++) {
        auto& pool = entityManager.components.pools[p];
        if (pool.size == 0) continue;
        auto poolSignature = pool.signature();
        if (poolSignature.hasAll(required)
         && poolSignature.hasNone(rejected)) {
            if (eligiblePools)
                eligiblePools->push_back(&pool);
            eligibleEntities += pool.size;
        }
    }
    return eligibleEntities;
}

void calculateJobStagesFromSource(System* system, int* stages, JobHandle source, int stage) {
    stages[source] = stage;

    // check for loops
    if (stage > 50) {
        LogError("Job dependency loop detected!");
        return;
    }

    auto& dependencies = system->jobDependencies[source];
    for (JobHandle sourceDependentOn : dependencies) {
        if (stages[sourceDependentOn] < stage + 1) {
            calculateJobStagesFromSource(system, stages, sourceDependentOn, stage + 1);
        }
    }
}

void calculateJobStages(System* system, TinyPtrVectorVector<System::ScheduledJob>* jobsByStage) {
    StackAllocate<int, 14> stages{(int)system->jobs.size(), -1};
    for (int i = 0; i < system->jobs.size(); i++) {
        Job* job = system->jobs[i].job;
        if (stages[i] == -1) {
            calculateJobStagesFromSource(system, stages, i, 0);
        }
    }

    for (int i = 0; i < system->jobs.size(); i++) {
        if (jobsByStage->size() <= stages[i]) jobsByStage->resize(stages[i]+1);
        (*jobsByStage)[stages[i]].push_back(&system->jobs[i]);
    }
}

int calculateSystemStages(System* source, int stage) {
    int highestStage = stage;
    for (System* dependentOnSource : source->systemDependencies) {
        if (highestStage < dependentOnSource->systemOrder) {
            highestStage = dependentOnSource->systemOrder;
        }
        if (dependentOnSource->systemOrder <= stage) {
            calculateSystemStages(dependentOnSource, stage + 1);
        }
    }
    source->systemOrder = highestStage;
    return highestStage;
}

// adds commands to commandBuffer
void executeJobChunk(const JobChunk& chunk, EntityCommandBuffer* commandBuffer, const EntityManager* entityManager) {
    alignas(Job) StackAllocate<char, 256> jobBuf{(int)chunk.job->size};
    Job* job = (Job*)jobBuf.data();
    memcpy((void*)job, (void*)chunk.job, chunk.job->size);
    job->commandBuffer = commandBuffer;
    void* componentArrays[8] = {nullptr};
    for (int i = 0; job->componentIDs[i] != 255; i++) {
        auto componentID = job->componentIDs[i];
        char* poolComponentArray = chunk.pool->getComponentArray(componentID);
        assert(poolComponentArray && "Archetype pool does not have this component!");

        // need to adjust to make the pointer point 'componentIndex' number of components behind itself,
        // so when indexBegin is added to the base index in the for loop,
        // the range is actually 0...chunkSize
        poolComponentArray -= chunk.indexBegin * entityManager->getComponentSize(componentID);
        componentArrays[i] = poolComponentArray;
    }
    job->componentArrays = componentArrays;
    job->entities = chunk.pool->entities - chunk.indexBegin;

    chunk.job->executeFunc(job, chunk.groupVars, chunk.indexBegin, chunk.indexEnd);
    for (int i = 0; i < chunk.job->nConditionalExecutions; i++) {
        auto& conditionalExecutions = chunk.job->conditionalExecutions[i];
        if (chunk.pool->signature().hasAll(conditionalExecutions.required)) {
            conditionalExecutions.execute(job, chunk.groupVars, chunk.indexBegin, chunk.indexEnd);
        }
    }
}

template<typename T>
using SharedQueue = moodycamel::ConcurrentQueue<T>;

using TaskQueue = SharedQueue<JobChunk>;

struct ThreadData {
    struct PerThread {
        int threadNumber;
        EntityCommandBuffer commandBuffer;
    };
    PerThread personal;
    const EntityManager* entityManager;
    struct SharedData {
        TaskQueue* taskQueue;
        std::atomic<bool> quit;
        AtomicCountdown tasksToComplete;
    };
    SharedData* shared;
};

struct Thread {
    ThreadData* threadData;
    Threads::ThreadID id;
};

int executeJobTask(EntityCommandBuffer* commandBuffer, const JobChunk& task, const EntityManager* entityManager) {
    executeJobChunk(task, commandBuffer, entityManager);

    return 0;
}

int jobThreadFunc(void* dataPtr) {
    ThreadData* threadData = (ThreadData*)dataPtr;
    auto& queue = threadData->shared->taskQueue;
    ThreadData::PerThread& personalData = threadData->personal;
    JobChunk task;
    auto* entityManager = threadData->entityManager;
    while (!threadData->shared->quit.load(std::memory_order_relaxed)) {
        if (queue->try_dequeue(task)) {
            int taskSuccess = executeJobTask(&personalData.commandBuffer, task, entityManager);
            int counter = threadData->shared->tasksToComplete.decrement();
            if (counter == 1) {
                // we could tell the main thread we are done
            }
        }
    }
    return 0;
}

void ECS::Systems::setupSystems(SystemManager& sysManager) {
    for (System* system : sysManager.systems) {
        if (system->systemOrder == System::NullSystemOrder) {
            calculateSystemStages(system, 0);
        }
    }
    
    std::sort(sysManager.systems.begin(), sysManager.systems.end(),
    [](System* lhs, System* rhs){
        return lhs->systemOrder >= rhs->systemOrder;
    });

    
    for (System* system : sysManager.systems) {
        int numJobs = system->jobs.size();
        if (numJobs > 0) {
            calculateJobStages(system, &system->stageJobs);
        }
    }

    sysManager.wasSetup = true;
}

void ECS::Systems::cleanupSystems(SystemManager& sysManager) {

}

void runSystemJobsSinglethreaded(SystemManager& sysManager, const TinyPtrVectorVector<System::ScheduledJob>& jobs, const std::vector<std::vector<const ArchetypePool*>>& groupPools) {
    for (int stage = jobs.size() - 1; stage >= 0; stage--) {
        auto& stageJobList = jobs[stage];
        for (auto& scheduledJob : stageJobList) {
            Job* job = scheduledJob->job;
            GroupID group = scheduledJob->group;
            auto& eligiblePools = groupPools[group];
            job->commandBuffer = &sysManager.unexecutedCommands;
            void* componentArrays[8] = {nullptr};
            job->componentArrays = componentArrays;
            int index = 0;
            for (const auto* pool : eligiblePools) {
                for (int i = 0; job->componentIDs[i] != 255; i++) {
                    auto componentID = job->componentIDs[i];
                    char* poolComponentArray = pool->getComponentArray(componentID);
                    assert(poolComponentArray && "Archetype pool does not have this component!");
                    componentArrays[i] = poolComponentArray - index;
                }
                job->entities = pool->entities - index;

                job->executeFunc(job, scheduledJob->args, index, index + pool->size);
                for (int i = 0; i < job->nConditionalExecutions; i++) {
                    auto& conditionalExecutions = job->conditionalExecutions[i];
                    if (pool->signature().hasAll(conditionalExecutions.required)) {
                        conditionalExecutions.execute(job, scheduledJob->args, index, index + pool->size);
                    }
                }
            }
        }
    }
}

void runSystemJobsParallel(SystemManager& sysManager, 
    const TinyPtrVectorVector<System::ScheduledJob>& jobs, 
    const std::vector<std::vector<const ArchetypePool*>>& groupPools,
    ArrayRef<Thread> threads, TaskQueue& taskQueue, AtomicCountdown& taskCounter)
{
    int totalStageEntities = 0;
    for (int stage = jobs.size() - 1; stage >= 0; stage--) {
        auto& stageJobList = jobs[stage];

        // break jobs up into chunks
        std::vector<JobChunk> jobThreadChunks;
        std::vector<JobChunk> mainThreadChunks; // chunks to be run on main thread (this one)
        std::vector<JobChunk> blockingChunks; // chunks that can only be run when no other jobs are running
        
        // split jobs into chunks while the stage is the same
        for (auto scheduledJob : stageJobList) {
            Job* job = scheduledJob->job;
            GroupID group = scheduledJob->group;
            auto& eligiblePools = groupPools[group];

            int groupEntityOffset = 0;
            for (const ArchetypePool* pool : eligiblePools) {
                if (pool->size == 0) continue;

                // break up into chunks if pool is large enough.
                std::vector<JobChunk>* chunks;
                if (job->mainThread || !job->parallelize) {
                    chunks = &mainThreadChunks;
                } else if (job->blocking) {
                    chunks = &blockingChunks;
                    assert(!job->parallelize && "Blocking job can't be parallelized!");
                } else {
                    chunks = &jobThreadChunks;
                }

                int ChunkSize = job->parallelize ? 512 : pool->size;
                
                int remainderChunkSize = pool->size % ChunkSize;
                if (remainderChunkSize > 0) {
                    JobChunk chunk = {
                        .job = job,
                        .pool = pool,
                        .indexBegin = groupEntityOffset,
                        .indexEnd = groupEntityOffset + remainderChunkSize,
                        .poolOffset = 0,
                        .groupVars = scheduledJob->args
                    };
                    chunks->push_back(chunk);
                }
                int additionalChunkCount = pool->size / ChunkSize;
                for (int i = 0; i < additionalChunkCount; i++) {
                    JobChunk chunk = {
                        .job = job,
                        .pool = pool,
                        .indexBegin = groupEntityOffset + remainderChunkSize,
                        .indexEnd = groupEntityOffset + ChunkSize + remainderChunkSize,
                        .poolOffset = i * ChunkSize + remainderChunkSize,
                        .groupVars = scheduledJob->args
                    };
                    chunks->push_back(chunk);
                }
                groupEntityOffset += pool->size;
            }
        }

        // add tasks to queue
        taskCounter.increment(jobThreadChunks.size());
        taskQueue.enqueue_bulk(jobThreadChunks.data(), jobThreadChunks.size());

        // do main thread jobs after we have queued up job thread jobs
        for (int i = 0; i < mainThreadChunks.size(); i++) {
            auto& chunk = mainThreadChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, &sysManager.unexecutedCommands, sysManager.entityManager);
        }

        // wait for tasks to finish
        taskCounter.wait();
        for (int i = 0; i < threads.size(); i++) {
            // add thread command buffer to unexecuted command list
            sysManager.unexecutedCommands.combine(threads[i].threadData->personal.commandBuffer);
        }

        // do blocking jobs after job threads have closed
        for (int i = 0; i < blockingChunks.size(); i++) {
            auto& chunk = blockingChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, &sysManager.unexecutedCommands, sysManager.entityManager);
        }
    }
}

void executeSystemParallel(SystemManager& sysManager, System* system, const std::vector<std::vector<const ArchetypePool*>>& groupPools, ArrayRef<Thread> threads, ThreadData::SharedData* threadData) {
    if (system->flushCommandBuffers) {
        sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
    }
    // systems still flush command buffers (if value is set) when disabled
    if (!system->enabled) return;

    system->BeforeExecution();

    int numJobs = system->jobs.size();
    if (numJobs > 0) {
        runSystemJobsParallel(sysManager, system->stageJobs, groupPools, threads, *threadData->taskQueue, threadData->tasksToComplete);
    }

    // all jobs executed
    system->AfterExecution();
}

void ECS::Systems::executeSystemSingleThreaded(SystemManager& sysManager, System* system, const std::vector<std::vector<const ArchetypePool*>>& groupPools) {
    if (system->flushCommandBuffers) {
        sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
    }
    // systems still flush command buffers (if value is set) when disabled
    if (!system->enabled) return;

    system->BeforeExecution();

    int numJobs = system->jobs.size();
    if (numJobs > 0) {
        runSystemJobsSinglethreaded(sysManager, system->stageJobs, groupPools);
    }

    // all jobs executed
    system->AfterExecution();
}

void ECS::Systems::executeSystems(SystemManager& sysManager) {
    if (!sysManager.wasSetup) {
        LogOnce(Error, "Did not set up system manager before executing!");
        return;
    }

    int systemCount = sysManager.systems.size();

    std::vector<std::vector<const ArchetypePool*>> groupPools;
    groupPools.resize(sysManager.groups.size());

    for (GroupID id = 0; id < sysManager.groups.size(); id++) {
        Group* group = &sysManager.groups[id];
        int totalJobEntities;
        if (group->trigger == Group::EntityInGroup) {
            totalJobEntities = findEligiblePools(
                group->group.signature, group->group.subtract, 
                *sysManager.entityManager,
                &groupPools[id]);
        } else {
            ArchetypePool* pool = group->watcherPool;
            if (!pool) {
                LogError("Watching group with no watcher pool!");
                continue; // won't execute
            }
            if (pool->size > 0) {
                groupPools[id].push_back(pool);
            }
            totalJobEntities = pool->size;
        }

        // make sure arrays are right size for group
        for (auto& array : group->arrays) {
            void*& data = *array.ptrToData;
            if (array.capacity < totalJobEntities) {
                size_t minCapacity = totalJobEntities;
                // use some buffer so we don't have to reallocate again unless 
                // entity count goes up a lotreduce allocation count
                size_t capacity = minCapacity + totalJobEntities/16; 
                data = realloc(data, capacity * array.typeSize);
                array.capacity = capacity;
            } 
            // check if we are wasting lots of memory
            else if (array.capacity > totalJobEntities * 4) {
                // shrink to a third
                data = realloc(data, array.capacity / 3 * array.typeSize);
            }
        }
    }

    // don't bother parallelizing if we can't use atleast two worker threads
    bool parallelize = sysManager.allowParallelization && Global.threadManager.unusedThreads() >= 2;

    if (parallelize) {
        SmallVector<Thread, 8> threads;

        auto concurrentQueue = SharedQueue<JobChunk>();

        ThreadData::SharedData* sharedThreadData = new ThreadData::SharedData {
            .taskQueue = &concurrentQueue,
            .quit = false,
            .tasksToComplete = {0}
        };

        // determine how many threads we should use
        // TODO: DEBUG: high thread count
        int idealThreadCount = 4;
        idealThreadCount = MIN(idealThreadCount, Global.threadManager.unusedThreads());
        assert(idealThreadCount > 0);

        threads.reserve(idealThreadCount);

        for (int i = 0; i < idealThreadCount; i++) {
            auto* threadData = new ThreadData{
                .personal = {
                    .threadNumber = i,
                    .commandBuffer = {},
                },
                .entityManager = sysManager.entityManager,
                .shared = sharedThreadData
            };
            Threads::ThreadID thread = Global.threadManager.openThread(jobThreadFunc, threadData);
            
            if (thread) {
                threads.push_back({threadData, thread});
            } else {
                delete threadData;
                // no point trying to keep opening threads after it already failed
                LogError("Failed to open thread!");
                break;
            }
        }

        for (int s = 0; s < systemCount; s++) {
            System* system = sysManager.systems[s];
            executeSystemParallel(sysManager, system, groupPools, threads, sharedThreadData);
        }

        for (int i = 0; i < threads.size(); i++) {
            sharedThreadData->quit.store(true);
            Global.threadManager.waitThread(threads[i].id);
        }

        for (auto& thread : threads) {
            delete thread.threadData;
        }
        delete sharedThreadData;
    } else {
        for (int s = 0; s < systemCount; s++) {
            System* system = sysManager.systems[s];
            executeSystemSingleThreaded(sysManager, system, groupPools);
        }
    }

    // clear watcher pools for next frame
    for (GroupID id = 0; id < sysManager.groups.size(); id++) {
        if (sysManager.groups[id].watcherPool != nullptr) {
            sysManager.groups[id].watcherPool->clear();
        }
    }

    // flush all commands at end of systems execution
    sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
}
