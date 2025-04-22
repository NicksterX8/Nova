
#include "My/Vec.hpp"
#include "ECS/generic/components.hpp"

struct AllocListRef {
    void* buffer;
    void** allocations;

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

namespace PECS {

struct IDGetter {
    template<class C>
    static constexpr Sint16 get() {
        return C::ID;
    }
};

#define GECS_MAX_COMPONENT 64
using EcsClass = GECS::Class<GECS_MAX_COMPONENT, IDGetter>;

using ComponentSignature = EcsClass::Signature;

using TypeID = Uint32;

struct Prototype {
    TypeID id = 0;
    ComponentSignature signature = 0;
    EcsClass::LargeComponentList components = EcsClass::LargeComponentList::New();

    GECS::ComponentInfoRef componentInfo;

    /* Customizable, universal components */

    

    /* Constructors */

    Prototype() {}

    Prototype(AllocListRef componentsStorage, GECS::ComponentInfoRef componentInfo, TypeID id, ComponentSignature signature = 0)
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

    bool has(GECS::ComponentID id) const {
        return signature[id];
    }

    template<class C>
    bool has() const {
        return signature[C::ID];
    }

    void* get(GECS::ComponentID component) {
        return components.get(component);
    }

    const void* get(GECS::ComponentID component) const {
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
};

struct PrototypeManager {
    GECS::ComponentInfoRef componentInfo; // default constructed to null
    std::vector<Prototype> prototypes;
    My::BucketArray<char, 1024> componentAllocator = My::BucketArray<char, 1024>::Empty();

    PrototypeManager() = default;

    PrototypeManager(GECS::ComponentInfoRef componentInfo, int numPrototypes) : componentInfo(componentInfo) {
        prototypes.resize(numPrototypes, Prototype());
    }

    const Prototype& get(TypeID id) const {
        return prototypes[id];
    }

    int numPrototypes() const {
        return prototypes.size();
    }

    // add prototype
    void add(const Prototype& prototype) {
        auto id = prototype.id;
        prototypes[id] = prototype;
    }

    AllocListRef allocateComponents(ComponentSignature signature) {
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

template<class... Components>
Prototype New(PrototypeManager* manager, TypeID id, Components... components) {
    constexpr auto signature = getComponentSignature<Components...>();
    auto componentsStorage = manager->allocateComponents(signature);
    auto prototype = Prototype(componentsStorage, manager->componentInfo, id, signature);
    FOR_EACH_VAR_TYPE(prototype.set<Components>(components...));
    return prototype;
}



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

using ComponentID = GECS::ComponentID;
using ComponentsAddress = GECS::ArchetypeElementAddress;
constexpr auto NullComponentsAddress = GECS::NullElementAddress;
using GECS::ComponentInfoRef;

struct Entity {
    ComponentSignature signature;
    ComponentsAddress componentsLoc;
    TypeID type;
 
    Entity() : signature(0), componentsLoc(NullComponentsAddress), type(0) {};

    Entity(TypeID type, ComponentSignature signature = 0, ComponentsAddress components = NullComponentsAddress) : signature(signature), componentsLoc(components), type(type) {}

    static Entity None() {
        return {0, ComponentSignature{0}, NullComponentsAddress};
    }

    template<class ...Cs>
    bool has() const {
        return has(getComponentSignature<Cs...>());
    }

    bool has(ComponentSignature components) const {
        return signature.hasAll(components);
    }

    bool has(ComponentID component) const {
        return signature[component];
    }
};

struct ComponentManager : EcsClass::ArchetypalComponentManager {
    ComponentManager() = default;
    ComponentManager(ComponentInfoRef info) : EcsClass::ArchetypalComponentManager(info) {}
};



struct EntityManager {
    ComponentManager    components;
    PrototypeManager    prototypes;

    EntityManager() = default;

    EntityManager(ComponentInfoRef componentInfo, int numPrototypes)
     : components(componentInfo), prototypes(componentInfo, numPrototypes) {} 
};


inline const Prototype* getPrototype(TypeID type, const EntityManager& manager) {
    //TODO: check for null
    if (type >= manager.prototypes.prototypes.size()) return nullptr;
    return &manager.prototypes.prototypes[type];
}

template<class C>
const C* getProtoComponent(const Entity& entity, const EntityManager& manager) {
    // TODO: check for null
    return getPrototype(entity.type, manager)->get<C>();
}

template<class C>
C* getRegularComponent(const Entity& entity, const EntityManager& manager) {
    return static_cast<C*>(manager.components.getComponent(entity.componentsLoc, C::ID));
}

template<class C>
typename std::conditional<C::PROTOTYPE, const C*, C*>::type getComponent(const Entity& entity, const EntityManager& manager) {
    if constexpr (C::PROTOTYPE) {
        return getProtoComponent<C>(entity, manager);
    } else {
        return getRegularComponent<C>(entity, manager);
    }
}

template<class C>
void setComponent(const Entity& entity, const EntityManager& manager, const C& value) {
    static_assert(C::PROTOTYPE == false);
    C* component = getComponent<C>(entity, manager);
    if (component) {
        *component = value;
    } else {
        LogError("Component %s does not exist for entity!", manager.components.componentInfo.name(C::ID));
    }
}

inline Entity makeEntity(TypeID type, EntityManager& manager) {
    auto* prototype = getPrototype(type, manager);
    if (!prototype) {
        LogError("NO prototype found for entity type %d!", type);
        return Entity::None();
    }
    ComponentSignature prototypeSignature = 0;
    if (prototype) {
        prototypeSignature = prototype->signature;
    }
    return Entity(type, prototypeSignature, NullComponentsAddress);
}

}