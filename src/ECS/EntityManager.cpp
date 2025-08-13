#include "ECS/EntityManager.hpp"

namespace ECS {

void EntityManager::init(ArrayRef<ComponentInfo> componentList, int numPrototypes) {
    this->componentInfo = NEW_ARR(ComponentInfo, componentList.size());
    nComponents = componentList.size();
    std::copy(componentList.begin(), componentList.end(), this->componentInfo);
    components.init(ArrayRef{componentInfo, (size_t)nComponents});
    prototypes.init(ArrayRef{componentInfo, (size_t)nComponents}, numPrototypes);
    stateLocked = new bool(false);
    commandBuffer = nullptr;
    componentDestructors = ComponentSet<ComponentDestructor>::Empty();
}

void EntityManager::executeCommandBuffer(EntityCommandBuffer* commandBuffer) {
    assert(commandBuffer);
    struct FakeEntity {
        EntityID fakeID;
        Entity real;
    };
    SmallVector<FakeEntity> fakeEntities;
    for (auto& command : commandBuffer->commands) {
        // check if the command is on a 'fake entity' 
        // one that is used only to represent an entity that will be made
        // sometime in the future
        
        // there can't be a real entity yet for an entity just being created
        if (command.type != EntityCommandBuffer::Command::CommandCreate 
            && command.entity.id > NullEntity.id) {
            // is fake entity
            for (auto& entity : fakeEntities) {
                if (command.entity.id == entity.fakeID) {
                    // substitute for real one
                    command.entity = entity.real;
                    goto realFound;
                }
            }
            // no real found
            LogError("no real?");
        }
    realFound:
        switch (command.type) {
        case EntityCommandBuffer::Command::CommandCreate: {
            Entity realEntity = createEntity(command.create.prototype);
            // made fake entity - register it so we can detect it in future commands
            fakeEntities.push_back({command.entity.id, realEntity});
            break; }
        case EntityCommandBuffer::Command::CommandAdd:
            doAddComponent(command.entity, command.add.component, command.add.componentValueIndex + commandBuffer->valueBuffer.data);
            break;
        case EntityCommandBuffer::Command::CommandAddSignature:
            addSignature(command.entity, command.addSignature.signature);
            break;
        case EntityCommandBuffer::Command::CommandSet: {
            void* component = getComponent(command.entity, command.set.component);
            void* value = &commandBuffer->valueBuffer[command.set.componentValueIndex];
            memcpy(component, value, getComponentSize(command.set.component));
            break; }
        case EntityCommandBuffer::Command::CommandRemove:
            doRemoveComponent(command.entity, command.remove.component);
            break;
        case EntityCommandBuffer::Command::CommandDelete:
            doDeleteEntity(command.entity);
            break;
        default:
            LogError("Invalid command type!");
        }
    }

    commandBuffer->destroy();
}

bool EntityManager::entityHas(Entity entity, Signature needComponents) const {
    if (entity.Null()) return false;

    const Prototype* prototype = getPrototype(entity);

    Signature signature = components.getEntitySignature(entity);
    if (prototype) {
        signature |= prototype->signature;
    }
    return (signature & needComponents) == needComponents;
}

bool EntityManager::addComponent(Entity entity, ComponentID component, const void* value) {
    if (!entityExists(entity)) {
        LogError("Entity does not exist!");
        return false;
    }

    if (commandsLocked()) {
        assert(commandBuffer && "Locking without valid command buffer!");
        commandBuffer->addComponent(entity, component, value, getComponentInfo(component).size);
        return false;
    }

    doAddComponent(entity, component, value);
    return true;
}

bool EntityManager::addSignature(Entity entity, Signature signature) {
    if (!entityExists(entity)) {
        LogError("Entity does not exist!");
        return false;
    }

    return components.addSignature(entity, signature);
}

 void EntityManager::destructComponents(Entity entity, Signature signature) {
    Signature neededDestructors = signature & componentsWithDestructors;
    auto& componentDestructors = this->componentDestructors;
    neededDestructors.forEachSet([&componentDestructors, entity](ComponentID component){
        auto* destructor = componentDestructors.lookup((Sint8)component);
        assert(destructor && "componentsWithDestructors and componentDestructors set mis match!");
        destructor->operator()(entity);
    });
}

void EntityManager::destructComponent(Entity entity, ComponentID component) {
    if (!componentsWithDestructors[component]) return;
    auto* destructor = componentDestructors.lookup((Sint8)component);
    assert(destructor && "componentsWithDestructors and componentDestructors set mis match!");
    destructor->operator()(entity);
}

void EntityManager::removeComponent(Entity entity, ComponentID component) {
    assert(validRegComponent(component));

    if (!entityExists(entity)) {
        LogError("Entity does not exist!");
    }

    if (commandsLocked()) {
        assert(commandBuffer && "Locking without valid command buffer!");
        commandBuffer->removeComponent(entity, component);
        return;
    }

    doRemoveComponent(entity, component);
}

void EntityManager::deleteEntity(Entity entity) {
    if (entity.Null()) {
        LogError("Cannot delete null entity!");
        SDL_TriggerBreakpoint();
        return;
    }

    if (commandsLocked()) {
        assert(commandBuffer && "Locking without valid command buffer!");
        commandBuffer->deleteEntity(entity);
        return;
    }

    doDeleteEntity(entity);
}

ComponentID EntityManager::getComponentIdFromName(const char* name) const {
    for (ComponentID id = 0; id < components.componentInfo.size(); id++) {
        if (My::streq(components.getComponentInfo(id).name, name)) {
            return id;
        }
    }
    return NullComponentID;
}

}