#ifndef ECS_GENERIC_ECS_INCLUDED
#define ECS_GENERIC_ECS_INCLUDED

#include <vector>
#include "My/Vec.hpp"
#include "ECS/generic/components.hpp"
#include "ECS/generic/ArchetypePool.hpp"
#include "ECS/Entity.hpp"

#define FLAG_COMPONENT(name) BEGIN_COMPONENT(name) END_COMPONENT(name)
#define FLAG_PROTO_COMPONENT(name) BEGIN_PROTO_COMPONENT(name) END_PROTO_COMPONENT(name)

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

namespace GECS {

struct IDGetter {
    template<class C>
    static constexpr Sint16 get() {
        return C::ID;
    }
};

#define GECS_MAX_COMPONENT 64
using EcsClass = GECS::Class<GECS_MAX_COMPONENT, IDGetter>;

using ComponentSignature = EcsClass::Signature; // 64 components or flags is probably good

// wrapper
template<class ...Cs>
constexpr ComponentSignature getComponentSignature() {
    return EcsClass::getSignature<Cs...>();
}

using PrototypeID = Sint32;

struct Prototype {
    PrototypeID id = 0;
    ComponentSignature signature = 0;
    EcsClass::LargeComponentList components = EcsClass::LargeComponentList::New();

    GECS::ComponentInfoRef componentInfo;

    My::Vec<void*> allocations = My::Vec<void*>::Empty();

    /* Customizable, universal components */

    /* Constructors */

    Prototype() {}

    Prototype(AllocListRef componentsStorage, ComponentInfoRef componentInfo, PrototypeID id, ComponentSignature signature = 0)
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
    GECS::ComponentInfoRef componentInfo; // default constructed to null
    std::vector<Prototype> prototypes;
    My::BucketArray<char, 1024> componentAllocator = My::BucketArray<char, 1024>::Empty();

    PrototypeManager() = default;

    PrototypeManager(GECS::ComponentInfoRef componentInfo, int numPrototypes) : componentInfo(componentInfo) {
        prototypes.resize(numPrototypes, Prototype());
    }

    template<class... Components>
    Prototype New(PrototypeID id, Components... components) {
        constexpr auto signature = getComponentSignature<Components...>();
        auto componentsStorage = allocateComponents(signature);
        auto prototype = Prototype(componentsStorage, componentInfo, id, signature);
        FOR_EACH_VAR_TYPE(prototype.set<Components...>(components));
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

using GECS::ComponentInfoRef;

struct ComponentManager : EcsClass::ArchetypalComponentManager {
    ComponentManager() = default;
    ComponentManager(ComponentInfoRef info) : EcsClass::ArchetypalComponentManager(info) {}
};

struct ElementManager {
    ComponentManager    components;
    PrototypeManager    prototypes;

    ElementManager() = default;

    ElementManager(ComponentInfoRef componentInfo, int numPrototypes)
     : components(componentInfo), prototypes(componentInfo, numPrototypes) {} 

    template<class... ReqComponents, class Func>
    void forEachElement(Func func) const {
        EcsClass::Signature reqSignature = EcsClass::getSignature<ReqComponents...>();
        for (Uint32 i = 0; i < components.pools.size; i++) {
            auto& pool = components.pools[i];
            auto signature = pool.archetype.signature;
            if ((signature & reqSignature) == reqSignature) {
                for (Uint32 e = 0; e < pool.elements.size; e++) {
                    Element element = pool.elements[e];
                    func(element);
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
                for (Uint32 e = 0; e < pool.elements.size; e++) {
                    void* components = pool.getComponents(e);
                    auto* offset = pool.archetype.getOffset(C::ID);
                    assert(offset);
                    C* component = (C*)((char*)components + *offset);
                    func(component);
                }
            }
        }
    }

    Element newElement(PrototypeID prototype) {
        return components.newElement(prototype);
    }

    inline const Prototype* getPrototype(PrototypeID type) const {
        return prototypes.get(type);
    }

    inline const Prototype* getPrototype(Element element) const {
        auto* elementData = components.getElementData(element.id);
        if (!elementData || (elementData->version != element.version)) {
            return nullptr;
        }
        return getPrototype(elementData->prototype);
    }

protected:
    template<class C>
    const C* getProtoComponent(Element element) const {
        auto* prototype = getPrototype(element);
        return prototype ? prototype->get<C>() : nullptr;
    }

    template<class C>
    C* getRegularComponent(Element element) const {
        return (C*)components.getComponent(element, C::ID);
    }
public:

    template<class C>
    typename std::conditional<C::PROTOTYPE, const C*, C*>::type getComponent(Element element) const {
        if constexpr (C::PROTOTYPE) {
            return getProtoComponent<C>(element);
        } else {
            return getRegularComponent<C>(element);
        }
    }

    template<class C>
    void setComponent(Element element, const C& value) {
        static_assert(C::PROTOTYPE == false);
        C* component = getComponent<C>(element);
        if (component) {
            *component = value;
        } else {
            LogError("Component %s does not exist for element!", components.componentInfo.name(C::ID));
        }
    }

    template<class C>
    bool addComponent(Element element, const C& startValue) {
        C* component = (C*)components.addComponent(element, C::ID);
        if (!component) return false;
        memcpy(component, &startValue, sizeof(C));
        return true;
    }

    template<class... Components>
    bool elementHas(Element element) const {
        if (element.Null()) return false;

        const Prototype* prototype = getPrototype(element);

        Signature signature = components.getElementSignature(element);
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