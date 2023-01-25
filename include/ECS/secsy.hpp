#include <stdlib.h>
#include <type_traits>
#include "llvm/SmallVector.h"
#include "My/Bitset.hpp"
#include "My/Vec.hpp"
#include "My/Allocation.hpp"
#include "llvm/ArrayRef.h"
#include "utils/templates.hpp"

namespace SECS {

typedef int16_t ComponentID;
#define MAX_NUM_COMPONENTS 64
//typedef My::Bitset<64> ComponentSignature;

template<typename T>
ComponentID _getID() {
    static ComponentID counter = 0;
    return counter++;
}

template<typename T>
ComponentID getID() {
    
}

struct ComponentType {
    int id;
};

struct AnyComponent {
    void* component;
    ComponentID id;

    /*
    AnyComponent() = default;

    AnyComponent(ComponentID id, size_t size) {
        this->component = malloc(size);
        this->id = id;
    }
    */

    void destroy() {
        free(component);
    }
};

template<typename T>
AnyComponent makeAnyComponent(const T& componentValue) {
    AnyComponent component;
    component.id = getID<T>();
    component.component = safe_malloc(sizeof(T));
    memcpy(component.component, &componentValue, sizeof(T));
    return component;
}

template<typename T>
AnyComponent makeAnyComponent() {
    AnyComponent component;
    component.id = getID<T>();
    component.component = malloc(sizeof(T));
    return component;
}

AnyComponent makeAnyComponent(ComponentID id, size_t componentSize) {
    return {.component = malloc(componentSize), .id = id};
}

struct ComponentData {
    ComponentID id;
    size_t size;
    void *(beforeDestroy)(void* param);
    void *(afterCreate)(void* param);
    My::DynAllocator allocator;
};

struct EntityManager {
    llvm::SmallVector<ComponentData, 0> m_components;

    const ComponentData* getComponent(ComponentID id) const {
        return &m_components[id];
    }

    int numComponents() const {
        return m_components.size();
    }

    template<typename T>
    T* allocComponents(size_t count) {

    }
};
/*
// possibly use some sort of bitset to be able to check if an entity has a component faster.
// Annoying because ids would need to be in some sort of range so we can index by them

// O(n) where n is the number of components for most operations involving it. Optimized for small numbers of components (0-4 or so)
struct BasicComponentList {
    llvm::SmallVector<AnyComponent, 3> list;

    template<typename T>
    const T* get() const {
        constexpr ComponentID id = getID<T>();
        for (auto& component : list) {
            if (component.id == id)
                return static_cast<T*>(component.component);
        }
        return nullptr; 
    }

    template<typename T>
    T* get() {
        return const_cast<T*>(static_cast<const BasicComponentList*>(this)->get<T>());
    }

    template<typename T>
    bool set(const T& value) {
        auto component = get<T>();
        if (component) {
            *component = value;
            return true;
        }
        return false;
    }
    
    template<typename T>
    int add(T* buffer) {
        // hope the component isn't getting added twice I guess.
        // It would take a long time to check if the component already exists by iterating though the vector
        list.push_back({buffer, getID<T>()});
        list.back() = value;
        return 0; // it could probably just return void cause no way of checking for failure
    }

    template<typename T>
    int remove() {
        constexpr ComponentID id = getID<T>();
        for (auto& component : list) {
            if (component.id == id) {
                list.erase(&component); // idk if this will work
                return 0;
            }
        }
        // couldn't find component in list
        return -1;
    }

    template<typename T>
    bool has() const {
        for (auto& component : list) {
            if (component.id == id)
                return true;
        }
        return false;
    }

    void destroy() {
        for (auto& component : list) {
            // TODO: maybe do 'beforeDestroy'
            component.destroy();
        }
    }

    const AnyComponent* getComponents() const {
        return list.data();
    }
};
*/

using AnyComponentList = llvm::SmallVector<AnyComponent, 1>;
using ComponentPtr = void*;
/*
template<size_t MaxComponentID = 64>
struct ComponentList {
    
    //ComponentID* componentIDs;
    //void** componentPtrs;

    
    using ComponentListInternal = My::VecTuple<ComponentID, ComponentPtr>;
    ComponentListInternal components;

    //int componentCount;
    //int componentCapacity;

    Signature signature;
};


void* getComponent(ComponentID componentID, int componentCount, const ComponentID* ids, void* const * components) {
    for (int c = 0; c < componentCount; c++) {
        if (ids[c] == componentID) {
            return components[c];
        }
    }
    return nullptr;
}


template<size_t MaxComponentID = 64>
ComponentList<MaxComponentID> makeComponentList(int initialCapacity) {
    ComponentList<MaxComponentID> list;
    list.components = ComponentList<MaxComponentID>::ComponentListInternal(initialCapacity);
    auto ptr = list.components.get<ComponentPtr>(0);
}

template<size_t MaxComponentID = 64>
struct ComponentListC {
    using Signature = My::Bitset<MaxComponentID>;
    My::VecTuple<ComponentID, ComponentPtr> components;
    Signature signature;

    template<typename T>
    const T* get() const {
        constexpr ComponentID id = getID<T>();
        for (int c = 0; c < components.size; c++) {
            if (components.get<ComponentID>(c) == id) {
                return static_cast<T*>(components.get<ComponentPtr>(c));
            }
        }
        return nullptr; 
    }

    template<typename T>
    T* get() {
        return const_cast<T*>(static_cast<const ComponentListC*>(this)->get<T>());
    }

    template<typename T>
    bool set(const T& value) {
        auto component = get<T>();
        if (component) {
            *component = value;
            return true;
        }
        return false;
    }
    
    template<typename T>
    int add(T* storage) {
        constexpr auto id = getID<T>();
        if (signature[id]) {
            return 1;
        }

        components.push(id, storage);

        return 0;
    }

    template<typename T>
    int remove() {
        constexpr ComponentID id = getID<T>();
        for (int c = 0; c < components.size; c++) {
            if (components.get<ComponentID>(c) == id) {
                list.erase(&component); // idk if this will work
                return 0;
            }
        }
        // couldn't find component in list
        return -1;
    }

    template<typename T>
    bool has() const {
        for (int c = 0; c < components.size; c++) {
            if (components.get<ComponentID>(c) == id)
                return true;
        }
        return false;
    }

    void destroy() {
        for (int c = 0; c < components.size; c++) {
            // TODO: maybe do 'beforeDestroy' or something
        }
    }
};
*/

/*
// max 32 components
struct SmallComponentList {
    using Signature = My::Bitset<32, uint32_t>;
    using ComponentVec = My::VecTuple<ComponentID, ComponentPtr>;

    ComponentVec* components;
    Signature signature;

    SmallComponentList() = default;

    SmallComponentList(Signature sig) : components(nullptr), signature(sig) {

    }

    SmallComponentList(Signature sig, int componentCount, const ComponentID* componentIDs, const ComponentPtr* componentPtrs) : signature(sig) {
        if (componentCount == 0) return;
        components = new ComponentVec(componentCount);
        components->push(componentCount, componentIDs, componentPtrs);
    }

    int getComponentCount() const {
        return (int)signature.count();
    }

    void makeVec(int capacity) {
        components = new ComponentVec(capacity);
    }

    void* get(ComponentID id) const {
        if (components) {
            for (int c = 0; c < components->size; c++) {
                if (components->get<ComponentID>(c) == id) {
                    return components->get<ComponentPtr>(c);
                }
            }
        }
        return nullptr;
    }

    bool has(ComponentID id) const {
        return signature[id];
    }

    void add(ComponentID id, void* storage) {
        if (storage) {
            if (!components)
                makeVec(1);
        }
    }

    void add(Signature componentSignature) {
        //signature |= componentSignature;
    }

    ComponentID* ids() const {
        if (components)
            return components->getPointer<ComponentID>();
        return nullptr;
    }

    ComponentPtr* ptrs() const {
        if (components)
            return components->getPointer<ComponentPtr>();
        return nullptr;
    }

    void destroy() {

    }
};
*/

/*
template<typename... Components> 
Entity makeEntity(EntityManager& manager) {
    /
    // allocate all components at once
    size_t totalSize = sumSizes<Components...>();
    void* components = malloc(totalSize);
    /

    // allocate each component one at a time
    constexpr size_t numComponents = sizeof...(Components);
    Entity entity;
    std::vector<AnyComponent> components;
    components.reserve(numComponents);
    FOR_EACH_VAR_TYPE(
        ([&](size_t componentSize, ComponentID id){
            components.push_back(makeAnyComponent(id, componentSize));
        })(sizeof(Components), getID<Components>()));

    return entity;
}

Entity makeEntity(EntityManager& manager, ArrayRef<ComponentID> componentIDs) {
    auto numComponents = componentIDs.size();
    Entity* entity = Alloc<Entity>();
    entity->m_components.reserve(numComponents);
    for (size_t i = 0; i < componentIDs.size(); i++) {
        ComponentID id = componentIDs[i];
        auto componentData = manager.getComponent(id);
        if (componentData)
            entity->m_components.push_back(makeAnyComponent(id, componentData->size));
    }
    return entity;
}
*/

/*
Probably not necessary to have a component base class
Actually if they all inherit from a base class we can use that to find bugs sooner,
so get<int> wouldn't work because it does not inherit from Component. 
*/


}