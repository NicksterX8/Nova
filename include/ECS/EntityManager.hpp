#ifndef ECS_GENERIC_ECS_INCLUDED
#define ECS_GENERIC_ECS_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ArchetypePool.hpp"
#include "Entity.hpp"
#include "PrototypeManager.hpp"
#include "CommandBuffer.hpp"

namespace ECS {

struct ComponentManager : ArchetypalComponentManager {
    ComponentManager() = default;
    ComponentManager(ComponentInfoRef info) : ArchetypalComponentManager(info) {}
};


struct EntityManager {
    ComponentManager components;
    PrototypeManager prototypes;

private:
    bool* stateLocked = new bool(false);

    EntityCommandBuffer tempCommandBuffer;
public:

    EntityManager() = default;

    EntityManager(ComponentInfoRef componentInfo, int numPrototypes)
     : components(componentInfo), prototypes(componentInfo, numPrototypes) {} 

    bool lock() const {
        if (!*stateLocked) {
            //*stateLocked = true;
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

    void executeCommandBuffer(EntityCommandBuffer& commandBuffer) {
        for (auto& command : commandBuffer.commands) {
            switch (command.type) {
            case EntityCommandBuffer::Command::CommandAdd:
                addComponent(command.value.add.target, command.value.add.component, command.value.add.componentValue);
                break;
            case EntityCommandBuffer::Command::CommandRemove:
                removeComponent(command.value.remove.target, command.value.remove.component);
                break;
            case EntityCommandBuffer::Command::CommandDelete:
                deleteEntity(command.value.del.target);
                break;
            default:
                LogError("Invalid command type!");
            }
        }

        commandBuffer.commands.clear();
    }

    template<class... ReqComponents, class Func>
    void forEachEntity(Func func) const {
        bool locked = lock();

        Signature reqSignature = getSignature<ReqComponents...>();
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

    // template<class C, class Func>
    // void forEachComponent(Func func) const {
    //     for (Uint32 i = 0; i < components.pools.size; i++) {
    //         auto& pool = components.pools[i];
    //         auto signature = pool.archetype.signature;
    //         if (signature[C::ID]) {
    //             auto index = pool.archetype.getIndex(C::ID);
    //             assert(index >= 0);
    //             C* components = (C*)pool.buffers[index];
    //             Entity* entities = pool.entities;
    //             for (Uint32 e = 0; e < pool.size; e++) {
    //                 func(entities[e], components[e]);
    //             }
    //         }
    //     }
    // }

    Entity newEntity(PrototypeID prototype) {
        Entity entity = components.newEntity(prototype);
        return entity;
    }

    void deleteEntity(Entity entity) {
        if (entity.Null()) {
            LogError("Cannot delete null entity!");
            return;
        }

        if (commandsLocked()) {
            tempCommandBuffer.deleteEntity(entity);
            return;
        }

        components.deleteEntity(entity);
    }

    bool entityExists(Entity entity) const {
        auto* entityData = components.getEntityData(entity.id);
        return entityData && (entityData->version == entity.version);
    }

    inline const Prototype* getPrototype(PrototypeID type) const {
        return prototypes.get(type);
    }

    inline const Prototype* getPrototype(Entity entity) const {
        auto* entityData = components.getEntityData(entity.id);
        if (!entityData || (entityData->version != entity.version)) {
            return nullptr;
        }
        return getPrototype(entityData->prototype);
    }

protected:
    template<class C>
    const C* getProtoComponent(Entity entity) const {
        auto* prototype = getPrototype(entity);
        return prototype ? prototype->get<C>() : nullptr;
    }

    template<class C>
    C* getRegularComponent(Entity entity) const {
        return (C*)components.getComponent(entity, C::ID);
    }
public:

    template<class C>
    typename std::conditional<C::PROTOTYPE, const C*, C*>::type getComponent(Entity entity) const {
        if constexpr (C::PROTOTYPE) {
            return getProtoComponent<C>(entity);
        } else {
            return getRegularComponent<C>(entity);
        }
    }

    // does not work for prototype components maybe TODO?
    void* getComponent(Entity entity, ComponentID component) const {
        return components.getComponent(entity, component);
    }

    template<class C>
    void setComponent(Entity entity, const C& value) {
        static_assert(C::PROTOTYPE == false);
        C* component = getComponent<C>(entity);
        if (component) {
            *component = value;
        } else {
            LogError("Component %s does not exist for entity!", components.componentInfo.name(C::ID));
        }
    }

    template<class C>
    bool addComponent(Entity entity, const C& startValue) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        if (commandsLocked()) {
            tempCommandBuffer.addComponent<C>(entity, startValue);
            return false;
        }

        C* component = (C*)components.addComponent(entity, C::ID);
        if (!component) return false;
        memcpy(component, &startValue, sizeof(C));
        return true;
    }

    // generic add. Not Lockable
    bool addComponent(Entity entity, ComponentID component, const void* value = nullptr) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        return (bool)components.addComponent(entity, component, value);
    }

    bool addSignature(Entity entity, Signature signature) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
            return false;
        }

        return components.addSignature(entity, signature);
    }

    template<class C>
    void removeComponent(Entity entity) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
        }

        if (commandsLocked()) {
            tempCommandBuffer.removeComponent<C>(entity);
            return;
        }

        components.removeComponent(entity, C::ID);
    }

    void removeComponent(Entity entity, ComponentID component) {
        if (!entityExists(entity)) {
            LogError("Entity does not exist!");
        }

        components.removeComponent(entity, component);
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

    void destroy() {
        components.destroy();
        prototypes.destroy();
    }
};

}

#endif