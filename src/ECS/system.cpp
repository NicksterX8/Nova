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
        auto poolSignature = pool.archetype.signature;
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
    StackAllocate<int, 14> stages{system->jobs.size(), -1};
    for (int i = 0; i < system->jobs.size(); i++) {
        Job* job = &system->jobs[i].job;
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
void executeJobChunk(const JobChunk& chunk, EntityCommandBuffer* commandBuffer) {
    JobData data{
        .pool = chunk.pool,
        .indexBegin = chunk.indexBegin,
        .dependencies = chunk.job->dependencies,
        .groupVars = chunk.groupVars,
        .commandBuffer = commandBuffer
    };

    chunk.job->executeFunc(&data, chunk.indexBegin, chunk.indexEnd);
    for (auto& conditionalExecutions : chunk.job->conditionalExecutions) {
        if (chunk.pool->signature().hasAll(conditionalExecutions.required)) {
            conditionalExecutions.execute(&data, chunk.indexBegin, chunk.indexEnd);
        }
    }
}

template<typename T>
using SharedQueue = moodycamel::ConcurrentQueue<T>;

struct ThreadData {
    struct PerThread {
        int threadNumber;
        EntityCommandBuffer commandBuffer;
    };
    PerThread personal;
    struct SharedData {
        SharedQueue<JobChunk>* taskQueue;
        std::atomic<bool> quit;
        AtomicCountdown tasksToComplete;
    };
    SharedData* shared;
};

int executeJobTask(ThreadData* threadData, const JobChunk& task) {
    int threadNumber = threadData->personal.threadNumber;
    EntityCommandBuffer* chunkCommandBuffer = &threadData->personal.commandBuffer;
    executeJobChunk(task, chunkCommandBuffer);

    return 0;
}

int jobThreadFunc(void* dataPtr) {
    ThreadData* threadData = (ThreadData*)dataPtr;
    auto& queue = threadData->shared->taskQueue;
    JobChunk task;
    while (!threadData->shared->quit.load(std::memory_order_relaxed)) {
        if (queue->try_dequeue(task)) {
            int taskSuccess = executeJobTask(threadData, task);
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
}

void ECS::Systems::cleanupSystems(SystemManager& sysManager) {

}

void runTriggerJob(SystemManager& sysManager, const System::ScheduledJob& job, Group::TriggerType trigger) {
    switch (trigger) {
    case Group::EntityEntered:
        break;
    case Group::EntityExited:
        break;
    }
}

void runSystemJobs(SystemManager& sysManager, const TinyPtrVectorVector<System::ScheduledJob>& jobs, const std::vector<std::vector<const ArchetypePool*>>& groupPools) {
    bool allowParallelization = sysManager.allowParallelization;

    SmallVector<Threads::ThreadID, 8> threads;
    SmallVector<ThreadData, 8> perThreadData;

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

    if (idealThreadCount > 0 && allowParallelization) {
        threads.reserve(idealThreadCount);
        perThreadData.resize(idealThreadCount, ThreadData{
            .shared = sharedThreadData,
        });

        for (int i = 0; i < idealThreadCount; i++) {
            perThreadData[i].personal.threadNumber = i;
            Threads::ThreadID thread = Global.threadManager.openThread(jobThreadFunc, &perThreadData[i]);
            if (thread) {
                threads.push_back(thread);
            } else {
                // no point trying to keep opening threads after it already failed
                break;
            }
        }
    }

    int entitiesInTask = 0;
    for (int stage = jobs.size() - 1; stage >= 0; stage--) {
        auto& stageJobList = jobs[stage];

        // break jobs up into chunks
        std::vector<JobChunk> jobThreadChunks;
        std::vector<JobChunk> mainThreadChunks; // chunks to be run on main thread (this one)
        std::vector<JobChunk> blockingChunks; // chunks that can only be run when no other jobs are running
        
        // split jobs into chunks while the stage is the same
        for (auto scheduledJob : stageJobList) {
            Job* job = &scheduledJob->job;
            GroupID group = scheduledJob->group;
            Group::TriggerType trigger = sysManager.getGroup(group)->trigger;
            if (trigger != Group::EntityInGroup) {
                runTriggerJob(sysManager, *scheduledJob, trigger);
            }
            auto& eligiblePools = groupPools[group];

            int groupEntityOffset = 0;
            for (const ArchetypePool* pool : eligiblePools) {
                if (pool->size == 0) continue;
                // assert(pool->size > 0 && "Empty eligible pool!");
                // break up into chunks if pool is large enough.
                // make it really small chunks so we can actually test
                std::vector<JobChunk>* chunks;
                if (job->mainThread || !job->parallelize || !allowParallelization) {
                    chunks = &mainThreadChunks;
                } else if (job->blocking) {
                    chunks = &blockingChunks;
                    assert(!job->parallelize && "Blocking job can't be parallelized!");
                } else {
                    chunks = &jobThreadChunks;
                }

                int ChunkSize = (job->parallelize && allowParallelization) ? pool->size / 64 : pool->size;
                
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

        if (threads.size() == 0 && !jobThreadChunks.empty()) {
            // give up on multithreading
            // make main thread run all chunks regardless of if they can be parallelized
            // add all job thread chunks to main thread chunks
            mainThreadChunks.insert(mainThreadChunks.end(), jobThreadChunks.begin(), jobThreadChunks.end());
            jobThreadChunks.clear();
        } else {
            // add tasks to queue
            sharedThreadData->tasksToComplete.increment(jobThreadChunks.size());
            concurrentQueue.enqueue_bulk(jobThreadChunks.data(), jobThreadChunks.size());
        }

        // do main thread jobs after we have queued up job thread jobs
        for (int i = 0; i < mainThreadChunks.size(); i++) {
            auto& chunk = mainThreadChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, &sysManager.unexecutedCommands);
        }

        // wait for tasks to finish
        sharedThreadData->tasksToComplete.wait();
        for (int i = 0; i < threads.size(); i++) {
            // add thread command buffer to unexecuted command list
            sysManager.unexecutedCommands.combine(perThreadData[i].personal.commandBuffer);
        }

        // do blocking jobs after job threads have closed
        for (int i = 0; i < blockingChunks.size(); i++) {
            auto& chunk = blockingChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, &sysManager.unexecutedCommands);
        }
    }

    for (int i = 0; i < threads.size(); i++) {
        sharedThreadData->quit.store(true);
        Global.threadManager.waitThread(threads[i]);
    }

    delete sharedThreadData;
}

void ECS::Systems::executeSystem(SystemManager& sysManager, System* system, const std::vector<std::vector<const ArchetypePool*>>& groupPools) {
    if (system->flushCommandBuffers) {
        sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
    }
    // systems still flush command buffers (if value is set) when disabled
    if (!system->enabled) return;

    system->BeforeExecution();

    int numJobs = system->jobs.size();
    if (numJobs > 0) {
        runSystemJobs(sysManager, system->stageJobs, groupPools);
    }

    // all jobs executed
    system->AfterExecution();
}

void ECS::Systems::executeSystems(SystemManager& sysManager) {
    // for (int i = 0; i < sysManager.groups.size(); i++) {
    //     auto& group = sysManager.groups[i];
    //     if (group.trigger == Group::EntityEntered) {
    //         std::vector<const ArchetypePool*> eligiblePools;
    //         findEligiblePools(group.group.signature, group.group.subtract, *sysManager.entityManager, &eligiblePools);
    //         for (auto* pool : eligiblePools) {
    //             const_cast<ArchetypePool*>(pool)->addedEntitiesWatchers.clear();
    //             const_cast<ArchetypePool*>(pool)->addedEntitiesWatchers.push_back(&sysManager.triggerGroupInfo[i].triggeredEntities);
    //         }
    //     }
    // }

    std::vector<std::vector<const ArchetypePool*>> groupPools;
    groupPools.resize(sysManager.groups.size());

    for (GroupID id = 0; id < sysManager.groups.size(); id++) {
        Group* group = &sysManager.groups[id];
        int totalJobEntities = findEligiblePools(
                group->group.signature, group->group.subtract, 
                *sysManager.entityManager,
                &groupPools[id]);

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

    int systemCount = sysManager.systems.size();
    for (int s = 0; s < systemCount; s++) {
        System* system = sysManager.systems[s];
        executeSystem(sysManager, system, groupPools);
    } // for each system end

    // flush all commands at end of systems execution
    sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
}
