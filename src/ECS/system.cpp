#include "ECS/system.hpp"
#include "llvm/SmallVector.h"
#include "global.hpp"
#include "moodycamel/concurrentqueue.h"

using namespace ECS;
using namespace ECS::System;


int ECS::System::findEligiblePools(const IComponentGroup& group, const ECS::EntityManager& entityManager, std::vector<ArchetypePool*>* eligiblePools) {
    int eligibleEntities = 0;
    for (int p = 0; p < entityManager.components.pools.size; p++) {
        auto& pool = entityManager.components.pools[p];
        if (pool.size == 0) continue;
        auto poolSignature = pool.archetype.signature;
        if ((poolSignature & group.signature) == group.signature
         && (poolSignature & group.subtract) == 0) {
            if (eligiblePools)
                eligiblePools->push_back(&pool);
            eligibleEntities += pool.size;
        }
    }
    return eligibleEntities;
}

int calculateJobStagesFromSource(ISystem* system, IJob* source, JobHandle sourceHandle, int stage, int* highestOfALL) {
    int highestStage = stage;
    auto& dependencies = system->jobDependencies[sourceHandle];
    for (JobHandle dependentOnSource : dependencies) {
        IJob* job = system->jobs[dependentOnSource].job;
        if (highestStage < job->stage) {
            highestStage = job->stage;
        }
        if (job->stage <= stage) {
            calculateJobStagesFromSource(system, job, dependentOnSource, stage + 1, highestOfALL);
        }
    }
    source->stage = highestStage;
    *highestOfALL = MAX(*highestOfALL, highestStage);
    return highestStage;
}

// returns highest stage
int calculateJobStages(ISystem* system) {
    int highestStage = -1;
    for (int i = 0; i < system->jobs.size(); i++) {
        IJob* job = system->jobs[i].job;
        if (job->stage == IJob::NullStage) {
            calculateJobStagesFromSource(system, job, i, 0, &highestStage);
        }
    }
    return highestStage;
}

int calculateSystemStages(ISystem* source, int stage) {
    int highestStage = stage;
    for (ISystem* dependentOnSource : source->systemDependencies) {
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

struct JobDistributor {
    static constexpr ThreadManager& threadManager = Global.threadManager;

    std::vector<JobChunk> chunksToExecute;
    using ThreadID = Threads::ThreadID;
    std::vector<ThreadID> threads;

    void init() {
        
    }

    /*
        input: a list of jobChunks that can all be executed in any order

        completes when all job chunks complete

        each thread needs it's own copy of the job so they can each
        have their own entity command buffer and component arrays references
    */
};

// do a clone of the job using UB if you know the size of the actual derived job
// must be freed with uglyCloneFree()
IJob* uglyClone(const IJob* job, int jobSize) {
    void* mem = SDL_aligned_alloc(CACHE_LINE_SIZE, jobSize);
    memcpy(mem, (void*)job, jobSize);
    return (IJob*)mem;
}

void uglyCloneFree(IJob* clone) {
    SDL_aligned_free(clone);
}

// adds commands to commandBuffer
// returns 0 on success, -1 on failure
// make sure two threads are not using the same copy of the job, make a copy of the job before for multithreading
// with executeJobChunkConcurrent
int executeJobChunk(const JobChunk& chunk, IJob* job, EntityCommandBuffer* commandBuffer) {
    JobData data(chunk, commandBuffer);
    job->init(job, data);
    if (data.failed()) {
        return -1;
    }
    job->executeRange(job, chunk.indexBegin, chunk.indexEnd);
    auto additionalExecutions = data.getAdditionalExecutions();
    for (auto& additionalExecution : additionalExecutions) {
        additionalExecution(job, chunk.indexBegin, chunk.indexEnd);
    }
    return 0;
}

int executeJobChunkConcurrent(const JobChunk& chunk, EntityCommandBuffer* commandsOut) {
    IJob* copy = uglyClone(chunk.job, chunk.job->realSize);

    int err = executeJobChunk(chunk, copy, commandsOut);

    uglyCloneFree(copy);
    return err;
}

struct JobThreadTask {
   JobChunk chunk;
};

template<typename T>
using SharedQueue = moodycamel::ConcurrentQueue<T>;

struct ThreadData {
    struct PerThread {
        int threadNumber;
        EntityCommandBuffer commandBuffer;
    };
    PerThread personal;
    struct SharedData {
        SharedQueue<JobThreadTask>* taskQueue;
        bool quit = false;
    };
    SharedData* shared;
};

int executeJobTask(ThreadData* threadData, const JobThreadTask& task) {
    int threadNumber = threadData->personal.threadNumber;
    EntityCommandBuffer* chunkCommandBuffer = &threadData->personal.commandBuffer;
    executeJobChunkConcurrent(task.chunk, chunkCommandBuffer);

    return 0;
}

int jobThreadFunc(void* dataPtr) {
    ThreadData* threadData = (ThreadData*)dataPtr;
    auto& queue = threadData->shared->taskQueue;
    JobThreadTask task;
    while (!threadData->shared->quit) {
        if (queue->try_dequeue(task)) {
            int taskSuccess = executeJobTask(threadData, task);
        } else {
            break;
        }
    }
    return 0;
}

void ECS::System::setupSystems(SystemManager& sysManager) {
    for (ISystem* system : sysManager.systems) {
        if (system->systemOrder == ISystem::NullSystemOrder) {
            calculateSystemStages(system, 0);
        }
    }
    
    std::sort(sysManager.systems.begin(), sysManager.systems.end(),
    [](ISystem* lhs, ISystem* rhs){
        return lhs->systemOrder >= rhs->systemOrder;
    });
}

void ECS::System::cleanupSystems(SystemManager& sysManager) {
    for (ISystem* system : sysManager.systems) {
        for (auto& scheduledJob : system->jobs) {
           delete scheduledJob.job;
        }
    }
}

void runSystemJobs(SystemManager& sysManager, ArrayRef<ISystem::ScheduledJob> jobs) {
    bool allowParallelization = sysManager.allowParallelization;

    int entitiesInTask = 0;
    int jobIndex = 0;
    for (int stage = jobs[0].job->stage; jobIndex < jobs.size(); stage--) {

        // break jobs up into chunks
        std::vector<JobChunk> jobThreadChunks;
        std::vector<JobChunk> mainThreadChunks; // chunks to be run on main thread (this one)
        std::vector<JobChunk> blockingChunks; // chunks that can only be run when no other jobs are running
        
        // split jobs into chunks while the stage is the same
        for (; jobIndex < jobs.size(); jobIndex++) {
            auto& scheduledJob = jobs[jobIndex];
            if (scheduledJob.job->stage < stage) {
                break;
            }
            IJob* job = scheduledJob.job;
            const IComponentGroup& group = scheduledJob.group;
            std::vector<ArchetypePool*> eligiblePools;
            int totalJobEntities = findEligiblePools(group, *sysManager.entityManager, &eligiblePools);

            int groupEntityOffset = 0;
            for (ArchetypePool* pool : eligiblePools) {
                // break up into chunks if pool is large enough.
                // make it really small chunks so we can actually test
                std::vector<JobChunk>* chunks;
                if (job->needMainThread || !job->parallelize || !allowParallelization) {
                    chunks = &mainThreadChunks;
                } else if (job->blocking) {
                    chunks = &blockingChunks;
                    assert(!job->parallelize && "Blocking job can't be parallelized!");
                } else {
                    chunks = &jobThreadChunks;
                }
                int ChunkSize = (job->parallelize && allowParallelization) ? pool->size : pool->size;
                
                int remainderChunkSize = pool->size % ChunkSize;
                if (remainderChunkSize > 0) {
                    JobChunk chunk = {
                        .job = job,
                        .pool = pool,
                        .indexBegin = groupEntityOffset,
                        .indexEnd = groupEntityOffset + remainderChunkSize,
                        .poolOffset = 0,
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
                        .poolOffset = i * ChunkSize + remainderChunkSize
                    };
                    chunks->push_back(chunk);
                }
                groupEntityOffset += pool->size;
            }
        }

        // determine how many threads we should use
        // TODO: DEBUG: high thread count
        int idealThreadCount = (int)ceil(jobThreadChunks.size() / 4.0f); // using incredibly high number for testing
        idealThreadCount = MIN(idealThreadCount, Global.threadManager.unusedThreads());

        llvm::SmallVector<Threads::ThreadID, 8> threads;
        llvm::SmallVector<ThreadData, 8> perThreadData;

        auto concurrentQueue = SharedQueue<JobThreadTask>();
        if (idealThreadCount > 0 && allowParallelization) {
            std::vector<JobThreadTask> tasks;
            tasks.reserve(jobThreadChunks.size());
            for (int i = 0; i < jobThreadChunks.size(); i++) {
                tasks.push_back(JobThreadTask{jobThreadChunks[i]});
            }

            // add tasks to queue
            concurrentQueue.enqueue_bulk(tasks.data(), tasks.size());

            ThreadData::SharedData sharedThreadData = ThreadData::SharedData {
                .taskQueue = &concurrentQueue,
                .quit = false
            };
            
            threads.reserve(idealThreadCount);
            perThreadData.resize(idealThreadCount, ThreadData{
                .shared = &sharedThreadData,
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

        if (threads.size() == 0 && !jobThreadChunks.empty()) {
            // give up on multithreading
            // make main thread run all chunks regardless of if they can be parallelized
            // add all job thread chunks to main thread chunks
            mainThreadChunks.insert(mainThreadChunks.end(), jobThreadChunks.begin(), jobThreadChunks.end());
            jobThreadChunks.clear();
        }

        // do main thread jobs after we have queued up job thread jobs
        for (int i = 0; i < mainThreadChunks.size(); i++) {
            auto& chunk = mainThreadChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, chunk.job, &sysManager.unexecutedCommands);
        }

        // wait for threads to finish jobs
        for (int i = 0; i < threads.size(); i++) {
            int ret = Global.threadManager.waitThread(threads[i]);
            // add thread command buffer to unexecuted command list
            sysManager.unexecutedCommands.combine(perThreadData[i].personal.commandBuffer);
        }

        // do blocking jobs after job threads have closed
        for (int i = 0; i < blockingChunks.size(); i++) {
            auto& chunk = blockingChunks[i];
            // put commands straight into unexecuted command list
            executeJobChunk(chunk, chunk.job, &sysManager.unexecutedCommands);
        }
    }
}

void ECS::System::executeSystem(SystemManager& sysManager, ISystem* system) {
    if (system->flushCommandBuffers) {
        sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
    }
    // systems still flush command buffers (if value is set) when disabled
    if (!system->enabled) return;

    system->BeforeExecution();

    system->ScheduleJobs();
    if (!system->jobs.empty()) {
        int highestStage = calculateJobStages(system);
        std::sort(system->jobs.begin(), system->jobs.end(), [](auto& lhs, auto& rhs){
            return lhs.job->stage >= rhs.job->stage;
        });

        runSystemJobs(sysManager, system->jobs);

        system->clearJobs();
    }
    
    for (void* allocation : system->tempAllocations) {
        Free(allocation);
    }
    system->tempAllocations.clear();

    // all jobs executed
    system->AfterExecution();
}

void ECS::System::executeSystems(SystemManager& sysManager) {
    int systemCount = sysManager.systems.size();
    for (int s = 0; s < systemCount; s++) {
        ISystem* system = sysManager.systems[s];
        executeSystem(sysManager, system);
    } // for each system end

    // flush all commands at end of systems execution
    sysManager.entityManager->executeCommandBuffer(&sysManager.unexecutedCommands);
}
