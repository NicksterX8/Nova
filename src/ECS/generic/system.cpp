#include "ECS/generic/system.hpp"

using namespace EcsSystem;

void ensureArrayCapacity(AbstractGroupArray* array, int arraySize) {
    int reservedSize = arraySize * array->typeSize;
    if (!array->memory) {
        array->memory = new My::Vec<char>(reservedSize);
    } else {
        array->memory->reserve(reservedSize);
    }
    array->memory->size = reservedSize;
}

using ArchetypePool = GECS::EcsClass::ArchetypalComponentManager::ArchetypePool;

void executeCommands(const std::vector<ElementCommandBuffer::Command>& commands) {

}

void EcsSystem::executeSystems(SystemManager& sysManager, GECS::ElementManager& elementManager) {

    std::vector<TaskGroup> tasks;

    // sort by system order
    std::sort(sysManager.systems.begin(), sysManager.systems.end(),
        [](const ISystem* a, const ISystem* b){
            return a->order < b->order;
        });

    for (ISystem* system : sysManager.systems) {
        if (!system->enabled) {
            continue;
        }

        system->ScheduleJobs();
        
        // sort system jobs by job order
        std::sort(system->schedule.begin(), system->schedule.end(),
            [](const ScheduledJob& a, const ScheduledJob& b){
                return a.order < b.order;
            });
        for (auto& job : system->schedule) {
            TaskGroup taskGroup{
                .jobs = {job.job},
                .group = *job.group,
                .system = system,
                .flushCommandBuffers = true
            };
            tasks.push_back(taskGroup);
        }
        system->schedule.clear();
    }

    std::vector<ElementCommandBuffer::Command> unexecutedCommands;

    for (TaskGroup& taskGroup : tasks) {
        if (taskGroup.flushCommandBuffers) {
            executeCommands(unexecutedCommands);
            unexecutedCommands.clear();
        }

        int elementsInTask = 0;
        std::vector<ArchetypePool*> eligiblePools;
        for (int p = 0; p < elementManager.components.pools.size; p++) {
            auto& pool = elementManager.components.pools[p];
            if (pool.size == 0) continue;
            auto poolSignature = pool.archetype.signature;
            if ((poolSignature & taskGroup.group.signature) == taskGroup.group.signature
        && (poolSignature & taskGroup.group.subtract) == 0) {
                eligiblePools.push_back(&pool);
                elementsInTask += pool.size;
            }
        }

        for (IJob* job : taskGroup.jobs) {
            for (AbstractGroupArray* array : job->arrays) {
                if (array->memory) {
                    ensureArrayCapacity(array, elementsInTask);
                }
            }
            int elementsProcessed = 0;
            for (ArchetypePool* pool : eligiblePools) {
                // make sure job arrays are referencing the correct data
                for (AbstractGroupArray* array : job->arrays) {
                    ComponentID neededComponent = array->componentType;
                    if (neededComponent != -1) {
                        if (!array->readonly && !taskGroup.group.write[neededComponent]) {
                            LogError("Component array without write permissions not marked read only. Review group permissions");
                        }
                        int neededComponentIndex = pool->archetype.getIndex(neededComponent);
                        assert(neededComponentIndex != -1);
                        char* poolComponentArray = pool->buffers[neededComponentIndex];
                        array->data = poolComponentArray;
                    } else if (array->memory) {
                        array->data = array->memory->data + elementsProcessed * array->typeSize;
                    } else if (array->elementArray) {
                        array->data = pool->elements;
                    } else {
                        LogError("Array doesn't seem to have any type. what up?");
                    }
                }

                // execute job per pool of elements
                for (int e = 0; e < pool->size; e++) {
                    job->Execute(e);
                }

                elementsProcessed += pool->size;
            }

            ElementCommandBuffer& commandBuffer = job->commands;
            unexecutedCommands.insert(unexecutedCommands.end(), commandBuffer.commands.begin(), commandBuffer.commands.end());
        } // for each job end
    } // for each task group end

    executeCommands(unexecutedCommands);
    unexecutedCommands.clear();

    // free arrays at some point
}

// template<class... Cs, class Func>
// void forEach(GECS::ComponentManager& componentManager, Func func, Signature subtract = {0}) {
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

