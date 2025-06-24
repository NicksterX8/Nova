#include "ECS/system.hpp"
#include "llvm/SmallVector.h"
#include "global.hpp"

using namespace ECS;
using namespace ECS::System;


int ECS::System::findEligiblePools(const IComponentGroup* group, const ECS::EntityManager& entityManager, std::vector<ArchetypePool*>* eligiblePools) {
    int eligibleEntities = 0;
    for (int p = 0; p < entityManager.components.pools.size; p++) {
        auto& pool = entityManager.components.pools[p];
        if (pool.size == 0) continue;
        auto poolSignature = pool.archetype.signature;
        if ((poolSignature & group->signature) == group->signature
         && (poolSignature & group->subtract) == 0) {
            if (eligiblePools)
                eligiblePools->push_back(&pool);
            eligibleEntities += pool.size;
        }
    }
    return eligibleEntities;
}

int calculateJobStages(IJob* source, int stage, int* highestOfALL) {
    int highestStage = stage;
    for (IJob* dependentOnSource : source->thisDependentOn.jobDependencies) {
        if (highestStage < dependentOnSource->stage) {
            highestStage = dependentOnSource->stage;
        }
        if (dependentOnSource->stage <= stage) {
            calculateJobStages(dependentOnSource, stage + 1, highestOfALL);
            *highestOfALL = MAX(*highestOfALL, dependentOnSource->stage);
        }
    }
    source->stage = highestStage;
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

int testThreads(void* ptr) {
    int cnt;

    for (cnt = 0; cnt < 10; ++cnt) {
        SDL_Delay(50);
    }

    return cnt;
}

void example() {
    ThreadManager& manager = Global.threadManager;

    auto coolThread = manager.openThread(testThreads, nullptr);

    int ret = manager.waitThread(coolThread);
    LogInfo("thread return: %d", ret);

    manager.closeThread(coolThread);
}

struct JobDistributor {
    static constexpr ThreadManager& threadManager = Global.threadManager;

    /*
        input: a list of jobChunks that can all be executed in any order

        completes when all job chunks complete

        each thread needs it's own copy of the job so they can each
        have their own entity command buffer and component arrays references
    */
};

void jobTask(IJob* job) {
    
    
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

    // do thread stuff
    

}

void ECS::System::cleanupSystems(SystemManager& sysManager) {
    for (ISystem* system : sysManager.systems) {
        for (IJob* job : system->jobs) {
            delete job;
        }
    }
}

struct JobChunk {
    IJob* job;
    ArchetypePool* pool;
    int poolOffset;
    int groupOffset;
    int size;
};

void ECS::System::executeSystems(SystemManager& sysManager) {

    int systemCount = sysManager.systems.size();
    
    
    std::vector<EntityCommandBuffer> unexecutedCommands;

    for (int i = 0; i < systemCount; i++) {
        ISystem* system = sysManager.systems[i];

        if (system->flushCommandBuffers) {
            for (auto& commandBuffer : unexecutedCommands) {
                sysManager.entityManager->executeCommandBuffer(&commandBuffer);
            }
            unexecutedCommands.clear();
        }
        // systems still flush command buffers (if value is set) when disabled
        if (!system->enabled) continue;

        system->BeforeExecution();

        system->ScheduleJobs();

        std::vector<IJob*> jobs;
        
        int highestStage = 0;

        for (IJob* job : system->jobs) {
            if (job->stage == IJob::NullStage) {
                calculateJobStages(job, 0, &highestStage);
            }
        }

        std::sort(system->jobs.begin(), system->jobs.end(), [](IJob* lhs, IJob* rhs){
            return lhs->stage >= rhs->stage;
        });

        int entitiesInTask = 0;

        std::vector<JobChunk> chunks;

        for (IJob* job : system->jobs) {
            JobGroup group = job->group;
            std::vector<ArchetypePool*> eligiblePools;
            findEligiblePools(group, *sysManager.entityManager, &eligiblePools);

            int groupEntityOffset = 0;
            for (ArchetypePool* pool : eligiblePools) {
                // break up into chunks if pool is large enough.
                // make it really small chunks so we can actually test
                constexpr int ChunkSize = 4;
                int remainderChunkSize = pool->size % ChunkSize;
                chunks.push_back({job, pool, 0, groupEntityOffset, remainderChunkSize});
                int additionalChunkCount = pool->size / ChunkSize;
                for (int i = 0; i < additionalChunkCount; i++) {
                    JobChunk chunk = {
                        .job = job,
                        .pool = pool,
                        .poolOffset = i * ChunkSize,
                        .groupOffset = groupEntityOffset,
                        .size = ChunkSize
                    };
                    chunks.push_back(chunk);
                }
                groupEntityOffset += pool->size;
            }

            unexecutedCommands.push_back(job->commands);
        }

        for (JobChunk& chunk : chunks) {
            IJob* job = chunk.job;
            ArchetypePool* pool = chunk.pool;

            llvm::SmallVector<OptionalExecuteType> optionalExecutes;
            // make sure job arrays are referencing the correct data
            for (AbstractGroupArray* array : job->arrays) {
                ComponentID neededComponent = array->componentType;
                if (neededComponent != -1) {
                    if (!job->group->signature[neededComponent]) {
                        LogError("Group not allowed to access component %d", neededComponent);
                    }
                    if (!array->readonly && !job->group->write[neededComponent]) {
                        LogError("Component array without write permissions not marked read only. Review group permissions");
                    }
                    int neededComponentIndex = pool->archetype.getIndex(neededComponent);
                    if (neededComponentIndex == -1) {
                        if (!array->optional) {
                            LogCritical("Couldn't get component pool for %d", neededComponent);
                            assert(0);
                        }
                    } else {
                        char* poolComponentArray = pool->getBuffer(neededComponentIndex);
                        Uint16 componentSize = pool->archetype.sizes[neededComponentIndex];
                        array->data = poolComponentArray - (chunk.groupOffset * componentSize);
                        if (array->optionalExecute) {
                            optionalExecutes.push_back(array->optionalExecute);
                        }
                    }
                } else if (array->entityArray) {
                    array->data = chunk.pool->entities - chunk.groupOffset;
                } else {
                    LogError("Array doesn't seem to have any type. what up?");
                }
            }

            // execute job per pool of entities
            for (int e = 0; e < chunk.size; e++) {
                job->Execute(chunk.groupOffset + chunk.poolOffset + e);
            }

            for (OptionalExecuteType optionalExecute : optionalExecutes) {
                for (int e = 0; e < chunk.size; e++) {
                    (job->*optionalExecute)(chunk.groupOffset + chunk.poolOffset + e);
                }
            }
        } // for each job end
        system->jobs.clear();

        // all jobs executed
        system->AfterExecution();

        for (void* allocation : system->tempAllocations) {
            Free(allocation);
        }
        system->tempAllocations.clear();
    } // for each system end

    for (auto& commandBuffer : unexecutedCommands) {
        sysManager.entityManager->executeCommandBuffer(&commandBuffer);
    }
    unexecutedCommands.clear();
}
