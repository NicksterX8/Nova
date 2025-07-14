#ifndef ECS_GENERIC_ECS_INCLUDED
#define ECS_GENERIC_ECS_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ArchetypePool.hpp"
#include "Entity.hpp"
#include "PrototypeManager.hpp"
#include "CommandBuffer.hpp"

namespace ECS {

using ComponentManager = ArchetypalComponentManager;

template<typename Value>
using ComponentSet = My::DenseSparseSet<Sint8, Value, Uint8, MaxComponentID>;

using ComponentDestructor = std::function<void(Entity)>;

struct EntityManager {
    ComponentManager components;
    PrototypeManager prototypes;

private:
    bool* stateLocked;
    EntityCommandBuffer* commandBuffer;

    Signature componentsWithDestructors = {0};
    ComponentSet<ComponentDestructor> componentDestructors;
public:

    void init(ComponentInfoRef componentInfo, int numPrototypes) {
        components.init(componentInfo);
        prototypes.init(componentInfo, numPrototypes);
        stateLocked = new bool(false);
        commandBuffer = nullptr;
        componentDestructors = ComponentSet<ComponentDestructor>::Empty();
    }

    const ComponentInfo& getComponentInfo(ComponentID component) const {
        return components.getComponentInfo(component);
    }

    void useCommandBuffer(EntityCommandBuffer* buffer) {
        assert(!commandBuffer && "Cannot use a new command buffer while another is active");
        this->commandBuffer = buffer;
    }

    // executes the current command buffer if it exists, and sets current command buffer to null
    void flushCurrentCommandBuffer() {
        executeCommandBuffer(commandBuffer);
        commandBuffer = nullptr;
    }

    // TODO: this is horrible... so slow and bad. figure out new solution for foreaches
    bool lock() const {
        if (!*stateLocked) {
            *stateLocked = true;
            return true;
        }
        return false;
    }

    void unlock() const {
        *stateLocked = false;
    }

    bool commandsLocked() const {
        return *stateLocked;
    }

    // command buffer is destroyed after this
    void executeCommandBuffer(EntityCommandBuffer* commandBuffer) {
        for (auto& command : commandBuffer->commands) {
            switch (command.type) {
            case EntityCommandBuffer::Command::CommandAdd:
                doAddComponent(command.value.add.target, command.value.add.component, command.value.add.componentValueIndex + commandBuffer->valueBuffer.data);
                break;
            case EntityCommandBuffer::Command::CommandRemove:
                doRemoveComponent(command.value.remove.target, command.value.remove.component);
                break;
            case EntityCommandBuffer::Command::CommandDelete:
                doDeleteEntity(command.value.del.target);
                break;
            default:
                LogError("Invalid command type!");
            }
        }

        commandBuffer->destroy();
    }

    template<class... ReqComponents, class Func>
    void forEachEntity(Func func) const {
        static_assert((!ReqComponents::PROTOTYPE && ...), "Prototype components cannot be used in for each!");
        bool locked = lock();

        constexpr Signature reqSignature = getSignatureNoProto<ReqComponents...>();

        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if ((signature & reqSignature) == reqSignature) {
                for (Uint32 e = 0; e < pool.size; e++) {
                    Entity entity = pool.entities[e];
                    func(entity);
                }
            }
        }

        if (locked) {
            unlock();
        }
    }

    template<class Func>
    void forAllEntities(Func func) const {
        bool locked = lock();

        int entityCount = getEntityCount();
        auto entityDataArray = components.entityData.getValueArray();
        auto entityIDArray = components.entityData.getKeyArray();
        for (int e = 0; e < entityCount; e++) {
            auto& data = entityDataArray[e];
            EntityID id = entityIDArray[e];
            if (func(Entity{id, data.version})) {
                break;
            }
        }

        if (locked) {
            unlock();
        }
    }

    template<class Query, class Func>
    void forEachEntity(Query query, Func func) const {
        bool locked = lock();

        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if (query(signature)) {
                for (int e = pool.size-1; e >= 0; e--) {
                    Entity entity = pool.entities[e];
                    func(entity);
                }
            }
        }

        if (locked) {
            unlock();
        }
    }

    template<class Query, class Func>
    void forEachEntity_EarlyReturn(Query query, Func func) const {
        bool locked = lock();
        
        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if (query(signature)) {
                for (int e = pool.size-1; e >= 0; e--) {
                    Entity entity = pool.entities[e];
                    if (func(entity)) {
                        return;
                    }
                }
            }
        }

        if (locked) {
            unlock();
        }
    }

    Entity newEntity(PrototypeID prototype) {
        Entity entity = components.newEntity(prototype);
        return entity;
    }

    // same as deleteEntity but no null checking
    // and no command buffering
    void doDeleteEntity(Entity entity) {
        destructComponents(entity, getEntitySignature(entity));
        components.deleteEntity(entity);
    }

    void deleteEntity(Entity entity) {
        if (entity.Null()) {
            LogError("Cannot delete null entity!");
            return;
        }

        if (commandsLocked()) {
            assert(commandBuffer && "Locking without valid command buffer!");
            commandBuffer->deleteEntity(entity);
            return;
        }

        doDeleteEntity(entity);
    }

    bool entityExists(Entity entity) const {
        auto* entityData = components.getEntityData(entity.id);
        return entityData && (entityData->version == entity.version);
    }

    const Prototype* getPrototype(PrototypeID type) const {
        return prototypes.get(type);
    }

    const Prototype* getPrototype(Entity entity) const {
        auto* entityData = components.getEntityData(entity.id);
        if (!entityData || (entityData->version != entity.version)) {
            return nullptr;
        }
        return getPrototype(entityData->prototype);
    }

    PrototypeID getPrototypeID(Entity entity) const {
        auto* entityData = components.getEntityData(entity.id);
        if (!entityData || (entityData->version != entity.version)) {
            return -1;
        }
        return entityData->prototype;
    }

    const Prototype* getPrototype(const char* prototypeName) const {
        return prototypes.get(prototypeName);
    }

protected:
    template<class C>
    const C* getProtoComponent(Entity entity) const {
        static_assert(C::PROTOTYPE, "Component must be a prototype component!");
        auto* prototype = getPrototype(entity);
        return prototype ? prototype->get<C>() : nullptr;
    }

    template<class C>
    C* getRegularComponent(Entity entity) const {
        static_assert(!C::PROTOTYPE, "Component must not be a prototype component!");
        return (C*)components.getComponent(entity, C::ID);
    }
public:

    template<class C>
    std::conditional_t<C::PROTOTYPE, const C*, C*> getComponent(Entity entity) const {
        if constexpr (C::PROTOTYPE) {
            return getProtoComponent<C>(entity);
        } else {
            return getRegularComponent<C>(entity);
        }
    }

    // get the component and assert that it exists. use this if you're not going to check if a component is null
    template<class C>
    std::conditional_t<C::PROTOTYPE, const C*, C*> getComponent_(Entity entity) const {
        std::conditional_t<C::PROTOTYPE, const C*, C*> component;
        if constexpr (C::PROTOTYPE) {
            component = getProtoComponent<C>(entity);
        } else {
            component = getRegularComponent<C>(entity);
        }
        assert(component != nullptr && "Component must not be null!");
        return component;
    }

    // does not work for prototype components maybe TODO?
    void* getComponent(Entity entity, ComponentID component) const {
        return components.getComponent(entity, component);
    }

    template<class C>
    void setComponent(Entity entity, const C& value) {
        static_assert(!C::PROTOTYPE, "Cannot set a prototype component value!");
        C* component = getComponent<C>(entity);
        if (component) {
            *component = value;
        } else {
            LogErrorLoc("Component %s does not exist for entity!", getComponentInfo(C::ID).name);
        }
    }

    bool doAddComponent(Entity entity, ComponentID component, const void* value) {
        // TODO: do on add
        return (bool)components.addComponent(entity, component, value);
    }

    template<class C>
    bool addComponent(Entity entity, const C& startValue) {
        static_assert(!C::PROTOTYPE, "Cannot add a prototype component to a entity!");
        
        return addComponent(entity, C::ID, &startValue);
    }

    // generic add
    bool addComponent(Entity entity, ComponentID component, const void* value = nullptr) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        if (commandsLocked()) {
            assert(commandBuffer && "Locking without valid command buffer!");
            commandBuffer->addComponent(entity, component, value, getComponentInfo(component).size);
            return false;
        }

        return (bool)doAddComponent(entity, component, value);
    }

    bool addSignature(Entity entity, Signature signature) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        return components.addSignature(entity, signature);
    }

protected:
    void destructComponents(Entity entity, Signature signature) {
        Signature neededDestructors = signature & componentsWithDestructors;
        auto& componentDestructors = this->componentDestructors;
        neededDestructors.forEachSet([&componentDestructors, entity](ComponentID component){
            auto* destructor = componentDestructors.lookup((Sint8)component);
            assert(destructor && "componentsWithDestructors and componentDestructors set mis match!");
            destructor->operator()(entity);
        });
    }

    void destructComponent(Entity entity, ComponentID component) {
        if (!componentsWithDestructors[component]) return;
        auto* destructor = componentDestructors.lookup((Sint8)component);
        assert(destructor && "componentsWithDestructors and componentDestructors set mis match!");
        destructor->operator()(entity);
    }
public:

    void setComponentDestructor(ComponentID component, const ComponentDestructor& destructor) {
        assert(!componentDestructors.contains(component) && "Component already has destructor set!");
        componentDestructors.insert(component, destructor);
        componentsWithDestructors.set(component);
    }

    bool validRegComponent(ComponentID component) {
        return components.validComponents[component];
    }

    bool validPrototypeComponent(ComponentID component) {
        return prototypes.validComponents[component];
    }

    template<class C>
    void removeComponent(Entity entity) {
        static_assert(!C::PROTOTYPE, "Cannot remove a prototype component from an entity!");
        removeComponent(entity, C::ID);
    }

    void doRemoveComponent(Entity entity, ComponentID component) {
        destructComponent(entity, component);
        components.removeComponent(entity, component);
    }

    void removeComponent(Entity entity, ComponentID component) {
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

    int getComponentSize(ComponentID component) const {
        return components.getComponentInfo(component).size;
    }

    template<class... Components>
    bool entityHas(Entity entity) const {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        const Prototype* prototype = getPrototype(entity);

        Signature signature = components.getEntitySignature(entity);
        if (prototype) {
            signature |= prototype->signature;
        }
        constexpr Signature componentSignature = getSignature<Components...>();
        return (signature & componentSignature) == componentSignature;
    }

    bool entityHas(Entity entity, Signature needComponents) const {
        if (entity.Null()) return false;

        const Prototype* prototype = getPrototype(entity);

        Signature signature = components.getEntitySignature(entity);
        if (prototype) {
            signature |= prototype->signature;
        }
        return (signature & needComponents) == needComponents;
    }

    Signature getEntitySignature(Entity entity) const {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return {0};
        }

        return components.getEntitySignature(entity);
    }

    int getEntityCount() const {
        return components.entityData.getSize();
    }

    void destroy() {
        components.destroy();
        prototypes.destroy();
    }

    ComponentID getComponentIdFromName(const char* name) const;
};

}

#endif