#include "ECS/system.hpp"

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
        for (IJob* job : system->jobs) {
            delete job;
        }
    }
}

void ECS::System::executeSystems(SystemManager& sysManager, ECS::EntityManager& entityManager) {

    int systemCount = sysManager.systems.size();
    
    
    std::vector<EntityCommandBuffer> unexecutedCommands;

    for (int i = 0; i < systemCount; i++) {
        ISystem* system = sysManager.systems[i];

        if (system->flushCommandBuffers) {
            for (auto& commandBuffer : unexecutedCommands) {
                entityManager.executeCommandBuffer(commandBuffer);
            }
        }
        // systems still flush command buffers (if value is set) when disabled
        if (!system->enabled) continue;

        system->ScheduleJobs();

        std::vector<IJob*> jobs;

        if (system->jobs.empty()) continue;
        
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
        std::vector<ArchetypePool*> eligiblePools;

        for (IJob* job : system->jobs) {
            JobGroup group = job->group;

            std::vector<ArchetypePool*> eligiblePools;
            findEligiblePools(group, entityManager, &eligiblePools);

            int entitiesProcessed = 0;
            for (ArchetypePool* pool : eligiblePools) {
                // make sure job arrays are referencing the correct data
                for (AbstractGroupArray* array : job->arrays) {
                    ComponentID neededComponent = array->componentType;
                    if (neededComponent != -1) {
                        if (!array->readonly && !group->write[neededComponent]) {
                            LogError("Component array without write permissions not marked read only. Review group permissions");
                        }
                        int neededComponentIndex = pool->archetype.getIndex(neededComponent);
                        assert(neededComponentIndex != -1);
                        char* poolComponentArray = pool->buffers[neededComponentIndex];
                        array->data = poolComponentArray - entitiesProcessed * pool->archetype.sizes[neededComponentIndex];
                    } else if (array->entityArray) {
                        array->data = pool->entities - entitiesProcessed;
                    } else {
                        LogError("Array doesn't seem to have any type. what up?");
                    }
                }

                // execute job per pool of entities
                for (int e = 0; e < pool->size; e++) {
                    job->Execute(entitiesProcessed + e);
                }

                entitiesProcessed += pool->size;
            }

            unexecutedCommands.push_back(job->commands);
        } // for each job end
        system->jobs.clear();
        for (void* allocation : system->tempAllocations) {
            Free(allocation);
        }
        system->tempAllocations.clear();
    } // for each system end

    for (auto& commandBuffer : unexecutedCommands) {
        entityManager.executeCommandBuffer(commandBuffer);
    }

    // free arrays at some point
}

// template<class... Cs, class Func>
// void forEach(ECS::ComponentManager& componentManager, Func func, Signature subtract = {0}) {
//     FOR_EACH_VAR_TYPE()
    
//     for (int p = 0; p < componentManager.pools.size; p++) {
//         auto& pool = componentManager.pools[p];
//         if (pool.size == 0) continue;
//         auto poolSignature = pool.archetype.signature;
//         if ((poolSignature & required) == required
//     && (poolSignature & subtract) == 0) {
//             for (AbstractComponentArray* componentArray : job->componentArrays) {
//                 ComponentID neededComponent = componentArray->type;
//                 if (!componentArray->readonly && !taskGroup.group.write[neededComponent]) {
//                     LogError("Component array without write permissions not marked read only. Review group permissions");
//                 }
//                 int neededComponentIndex = pool.archetype.getIndex(neededComponent);
//                 char* poolComponentArray = pool.buffers[neededComponentIndex];
//                 componentArray->data = poolComponentArray;
//             }

//             for (int e = 0; e < pool->size; e++) {
//                 func(e, components)
//             }
//         }
//     }
// }

