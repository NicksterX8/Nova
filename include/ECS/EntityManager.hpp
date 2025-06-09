#ifndef ECS_GENERIC_ECS_INCLUDED
#define ECS_GENERIC_ECS_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ArchetypePool.hpp"
#include "Entity.hpp"

#define FLAG_COMPONENT(name) BEGIN_COMPONENT(name) END_COMPONENT(name)
#define FLAG_PROTO_COMPONENT(name) BEGIN_PROTO_COMPONENT(name) END_PROTO_COMPONENT(name)

struct AllocListRef {
    void* buffer;
    void** allocations;

    AllocListRef() {
        buffer = nullptr;
        allocations = nullptr;
    }

    AllocListRef(void* buffer, ArrayRef<size_t> offsets) : buffer(buffer) {
        allocations = Alloc<void*>(offsets.size());
        for (int i = 0; i < offsets.size(); i++) {
            allocations[i] = (char*)buffer + offsets[i];
        }
    }

    void destroy() {
        Free(allocations);
    }
};

namespace ECS {

struct IDGetter {
    template<class C>
    static constexpr Sint16 get() {
        return C::ID;
    }
};

using PrototypeID = Sint32;

struct Prototype {
    PrototypeID id = 0;
    Signature signature = {0};
    LargeComponentList components = LargeComponentList::New();

    ComponentInfoRef componentInfo;

    My::Vec<void*> allocations = My::Vec<void*>::Empty();

    /* Customizable, universal components */

    /* Constructors */

    Prototype() {}

    Prototype(AllocListRef componentsStorage, ComponentInfoRef componentInfo, PrototypeID id, Signature signature = {})
    : id(id), signature(signature), componentInfo(componentInfo) {
        int i = 0;
        signature.forEachSet([&](auto componentID){
            components.add(componentID, componentsStorage.allocations[i]);
            i++;
        });
        componentsStorage.destroy();
    }

    virtual ~Prototype() {}

    /* Methods */
    /* Virtual */
    // these could be std::functions in the future
    virtual void onDestroy() {}
    virtual void onCreate() {}

    /* Regular */

    bool has(ComponentID id) const {
        return signature[id];
    }

    template<class C>
    bool has() const {
        return signature[C::ID];
    }

    void* get(ComponentID component) {
        return components.get(component);
    }

    const void* get(ComponentID component) const {
        return components.get(component);
    }

    template<class C>
    C* get() {
        return (C*)components.get(C::ID);
    }

    template<class C>
    const C* get() const {
        return (C*)components.get(C::ID);
    }

    template<class C>
    void set(const C& value) {
        auto component = get<C>();
        if (component) {
            *component = value;
        } else {
            LogError("Attempted to set unowned prototype component (%s)", componentInfo.name(C::ID));
        }
    }

    template<class C>
    void add(const C& value) {
        auto component = get<C>();
        if (component) {
            *component = value;
            LogWarn("Component (%s) already attached to this prototype!", componentInfo.name(C::ID));
        } else {
            C* storage = Alloc<C>(1);
            allocations.push(storage);
            memcpy(storage, &value, sizeof(C));
            components.add(C::ID, storage);
            signature.set(C::ID);
        }
    }

    void destroy() {
        components.destroy();

        for (auto ptr : allocations) {
            Free(ptr);
        }
        allocations.destroy();
    }
};

struct PrototypeManager {
    ComponentInfoRef componentInfo; // default constructed to null
    std::vector<Prototype> prototypes;
    My::BucketArray<char, 1024> componentAllocator = My::BucketArray<char, 1024>::Empty();

    PrototypeManager() = default;

    PrototypeManager(ComponentInfoRef componentInfo, int numPrototypes) : componentInfo(componentInfo) {
        prototypes.resize(numPrototypes, Prototype());
    }

    Prototype New(PrototypeID id) {
        //constexpr auto signature = getComponentSignature<Components...>();
        auto prototype = Prototype(AllocListRef(), componentInfo, id, {0});
        //FOR_EACH_VAR_TYPE(prototype.set<Components>(components, ...));
        return prototype;
    }

    const Prototype* get(PrototypeID id) const {
        if (id >= prototypes.size() || id < 0) return nullptr;
        return &prototypes[id];
    }

    int numPrototypes() const {
        return prototypes.size();
    }

    // add prototype
    void add(const Prototype& prototype) {
        auto id = prototype.id;
        prototypes[id] = prototype;
    }

    AllocListRef allocateComponents(Signature signature) {
        const auto count = signature.count();
        size_t* offsets = (size_t*)alloca(count * sizeof(size_t));
        size_t totalSize = 0;
        int i = 0;
        signature.forEachSet([&](auto componentID){
            const auto componentSize = componentInfo.size(componentID);
            const auto componentAlignment = componentInfo.alignment(componentID);
            const auto offset = totalSize + totalSize % componentAlignment;
            offsets[i] = offset;
            totalSize += offset;
            i++;
        });
        auto buffer = componentAllocator.reserveBack(totalSize);
        memset(buffer, 0, totalSize); // unnecessary, just for safety
        return AllocListRef(buffer, {offsets, count});
    }

    void destroy() {
        for (auto& prototype : prototypes) {
            prototype.components.destroy();
        }
        prototypes = std::vector<Prototype>(); // deallocate vector explicitly, in case the destructor is never called
        componentAllocator.destroy();
    }
};

#define BEGIN_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = false;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define BEGIN_PROTO_COMPONENT(name) struct name {\
    constexpr static ComponentID ID = ComponentIDs::name;\
    constexpr static bool PROTOTYPE = true;\
    constexpr static const char* NAME = TOSTRING(name);

#define END_PROTO_COMPONENT(name) }; static_assert(true | ComponentIDs::name, "Checking for id");

#define FLAG_COMPONENT(name) BEGIN_COMPONENT(name) END_COMPONENT(name)
#define FLAG_PROTO_COMPONENT(name) BEGIN_PROTO_COMPONENT(name) END_PROTO_COMPONENT(name)



struct ComponentManager : ArchetypalComponentManager {
    ComponentManager() = default;
    ComponentManager(ComponentInfoRef info) : ArchetypalComponentManager(info) {}
};

struct EntityCommandBuffer {
    struct Command {

        struct Add {
            Entity target;
            ComponentID component;
            void* componentValue;
        };

        struct Remove {
            Entity target;
            ComponentID component;
        };

        union {
            Add add;
            Remove remove;
        } value;

        enum CommandType {
            CommandAdd,
            CommandRemove
        } type;
    };

    std::vector<Command> commands;

    template<class C>
    void addComponent(Entity entity, const C& value) {
        // TODO: use better allocation method
        C* componentStorage = new C(value);
        commands.push_back(Command{
            .type = Command::CommandAdd,
            .value.add = {
                .target = entity,
                .component = C::ID,
                .componentValue = componentStorage
            }
        });
    }

    template<class C>
    void removeComponent(Entity entity) {
        commands.push_back(Command{
            .type = Command::CommandRemove,
            .value.remove = {
                .target = entity,
                .component = C::ID
            }
        });
    }
};

struct EntityManager {
    ComponentManager    components;
    PrototypeManager    prototypes;

    EntityManager() = default;

    EntityManager(ComponentInfoRef componentInfo, int numPrototypes)
     : components(componentInfo), prototypes(componentInfo, numPrototypes) {} 

    void executeCommandBuffer(EntityCommandBuffer& commandBuffer) {
        for (auto& command : commandBuffer.commands) {
            switch (command.type) {
            case EntityCommandBuffer::Command::CommandAdd:
                addComponent(command.value.add.target, command.value.add.component, command.value.add.componentValue);
                break;
            case EntityCommandBuffer::Command::CommandRemove:
                removeComponent(command.value.remove.target, command.value.remove.component);
                break;
            default:
                LogError("Invalid command type!");
            }
        }

        commandBuffer.commands.clear();
    }

    template<class... ReqComponents, class Func>
    void forEachEntity(Func func) const {
        EcsClass::Signature reqSignature = EcsClass::getSignature<ReqComponents...>();
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
    }

    template<class Query, class Func>
    void forEachEntity(Query query, Func func) const {
        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if (query(signature)) {
                for (Uint32 e = 0; e < pool.size; e++) {
                    Entity entity = pool.entities[e];
                    func(entity);
                }
            }
        }
    }

    template<class C, class Func>
    void forEachComponent(Func func) const {
        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if (signature[C::ID]) {
                for (Uint32 e = 0; e < pool.size; e++) {
                    // TODO:
                }
            }
        }
    }

    Entity newEntity(PrototypeID prototype) {
        Entity entity = components.newEntity(prototype);
        return entity;
    }

    bool entityExists(Entity entity) {
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
        C* component = (C*)components.addComponent(entity, C::ID);
        if (!component) return false;
        memcpy(component, &startValue, sizeof(C));
        return true;
    }

    // generic add
    bool addComponent(Entity entity, ComponentID component, const void* value = nullptr) {
        return (bool)components.addComponent(entity, component, value);
    }

    template<class C>
    void removeComponent(Entity entity) {
        components.removeComponent(entity, C::ID);
    }

    void removeComponent(Entity entity, ComponentID component) {
        components.removeComponent(entity, component);
    }

    template<class... Components>
    bool entityHas(Entity entity) const {
        if (entity.Null()) return false;

        const Prototype* prototype = getPrototype(entity);

        Signature signature = components.getEntitySignature(entity);
        if (prototype) {
            signature |= prototype->signature;
        }
        constexpr Signature componentSignature = getComponentSignature<Components...>();
        return (signature & componentSignature) == componentSignature;
    }

    void destroy() {
        components.destroy();
        prototypes.destroy();
    }
};

}

#endif