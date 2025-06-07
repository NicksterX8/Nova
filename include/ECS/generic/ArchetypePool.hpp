#ifndef ECS_ARCHETYPE_POOL_INCLUDED
#define ECS_ARCHETYPE_POOL_INCLUDED

#include <array>
#include "llvm/ArrayRef.h"
#include "My/SparseSets.hpp"
#include "My/HashMap.hpp"
#include "My/Bitset.hpp"
#include "utils/ints.hpp"
#include "ECS/Entity.hpp"

template <size_t I, typename Tuple>
constexpr size_t element_offset() {
    using element_t = std::tuple_element_t<I, Tuple>;
    static_assert(!std::is_reference<element_t>::value, "no references");
    union {
        char a[sizeof(Tuple)];
        Tuple t{};
    };
    auto* p = std::addressof(std::get<I>(t));
    t.~Tuple();
    std::size_t off = 0;
    for (std::size_t i = 0;; ++i) {
        if (static_cast<void*>(a + i) == p) return i;
    }
}

template <typename T, typename Tuple>
constexpr size_t element_offset() {
    union {
        char a[sizeof(Tuple)];
        Tuple t{};
    };
    using element_t = decltype(std::get<T>());
    static_assert(!std::is_reference<element_t>::value, "no references");
    auto* p = std::addressof(std::get<T>(t));
    t.~Tuple();
    std::size_t off = 0;
    for (std::size_t i = 0;; ++i) {
        if (static_cast<void*>(a + i) == p) return i;
    }
}

namespace GECS {

using Element = ECS::Entity;
using ElementID = ECS::EntityID;
using ElementVersion = ECS::EntityVersion;

constexpr Element NullElement = ECS::NullEntity;

constexpr ComponentID NullComponentID = INT16_MIN;
using ComponentPtr = void*;

struct ComponentInfo {
    size_t size = 0;
    size_t alignment = 0;
    const char* name = "null";
};

template<class Component>
static constexpr ComponentInfo getComponentInfo() {
    return {sizeof(Component), alignof(Component), Component::NAME};
}

template<class... Components>
static constexpr std::array<ComponentInfo, sizeof...(Components)> getComponentInfoList() {
    return {getComponentInfo<Components>() ...};
}

class ComponentInfoRef {
    ArrayRef<ComponentInfo> components;
public:
    ComponentInfoRef() {}

    ComponentInfoRef(ArrayRef<ComponentInfo> componentInfoList) : components(componentInfoList) {
        
    }

    int numComponents() const {
        return components.size();
    }

    size_t size(ComponentID component) const {
        return components[component].size;
    }

    size_t alignment(ComponentID component) const {
        return components[component].alignment;
    }

    const char* name(ComponentID component) const {
        return components[component].name;
    }
};

template<size_t maxComponentID, class IDGetter>
struct Class {

using Signature = Signature<maxComponentID>;

template<class C>
constexpr static ComponentID getRawID() {
    return IDGetter::template get<C>();
}

template<class C>
constexpr static ComponentID getID() {
    constexpr auto raw = getRawID<C>();
    return raw ? raw : -raw;
}

template<class C>
constexpr static bool isPrototype() {
    return getRawID<C>() < 0;
}

template<class C>
constexpr static bool getPrototypeID() {
    static_assert(getID<C>() != NullComponentID, "Bad component!");
    static_assert(getID<C>() < 0, "Component isn't a prototype component!");
    return getID<C>();
}

template<class ...Cs>
constexpr static Signature getSignature() {
    constexpr ComponentID ids[] = {getID<Cs>() ...};
    // sum component signatures
    auto result = Signature(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size())
            result.set(ids[i]);
    }
    return result;
}

template<class ...RegCs>
static Signature getNonProtoSignature(Signature signature) {
    constexpr Signature allRegularComponents = getSignature<RegCs...>();
    return signature & allRegularComponents;
}

struct ArchetypalComponentManager {
    struct Archetype {
        //using ComponentIndexSet = My::DenseSparseSet<ComponentID, Sint32, Sint8, maxComponentID>;
        Sint8 componentIndices[maxComponentID];
        int32_t numComponents;
        Signature signature;
        uint16_t* sizes;
        ComponentID* componentIDs;

        static Archetype Null() {
            return {{-1}, 0, 0, 0, nullptr};
        }

        Sint8 getIndex(ComponentID component) const {
            if (component < 0 || component >= maxComponentID) return -1;
            return componentIndices[component];
        }

        Uint16 getSize(ComponentID component) const {
            return sizes[componentIndices[component]];
        }
    };

    // template<class... Components>
    // static Archetype makeArchetype() {
    //     using Tuple = std::tuple<Components...>();

    //     Archetype archetype;
    //     constexpr auto count = sizeof...(Components);
    //     archetype.signature = getSignature<Components...>();
    //     archetype.numComponents = count;
    //     archetype.sizes = Alloc<uint16_t>(count);
    //     archetype.componentIDs = Alloc<ComponentID>(count);
    //     archetype.componentIndices = {0};
    //     constexpr auto sizes   = {sizeof(Components) ...};
    //     constexpr auto ids     = {getID<Components>() ...};
    //     for (int i = 0; i < count; i++) {
    //         archetype.sizes[i] = sizes[i];
    //         archetype.componentIDs[i] = ids[i];
    //         archetype.componentIndices[ids[i]] = i;
    //     }
    // }

    static Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo) {
        auto count = signature.count();
        Archetype archetype{
            .signature = signature,
            .componentIndices = {-1},
            .sizes = Alloc<uint16_t>(count),
            .componentIDs = Alloc<ComponentID>(count),
            .numComponents = (int)count
        };
        for (int i = 0; i < maxComponentID; i++) {
            archetype.componentIndices[i] = -1;
        }

        int index = 0;
        signature.forEachSet([&](auto componentID){
            auto componentSize = (uint16_t)componentTypeInfo.size(componentID);
            archetype.sizes[index] = componentSize;
            archetype.componentIDs[index] = componentID;
            archetype.componentIndices[componentID] = index++;
        });
        return archetype;
    }

    static constexpr size_t MaxElementID = (1 << 14) - 1;

    struct ArchetypePool {
        int size;
        int capacity;
        Element* elements; // contained elements
        My::Vec<char*> buffers;
        Archetype archetype;
        
        ArchetypePool(const Archetype& archetype) : archetype(archetype) {
            elements = nullptr;
            buffers = My::Vec<char*>::Filled(archetype.numComponents, nullptr);
            size = 0;
            capacity = 0;
        }

        // index must be in bounds
        void* getComponent(ComponentID component, int index) const {
            assert(index < size);

            auto componentIndex = archetype.getIndex(component);
            if (componentIndex == -1) return nullptr;
            return buffers[componentIndex] + index * archetype.sizes[componentIndex];
        }

        // returns index where element is stored
        int addNew(Element element) {
            if (size + 1 > capacity) {
                int newCapacity = (capacity * 2 > size) ? capacity * 2 : size + 1;
                for (int i = 0; i < buffers.size; i++) {
                    char* newBuffer = Realloc(buffers[i], newCapacity * archetype.sizes[i]);
                    if (newBuffer) {  
                        buffers[i] = newBuffer;
                    }
                }
                Element* newElements = Realloc(elements, newCapacity);
                if (newElements) {
                    elements = newElements;
                }
                capacity = newCapacity;
            }

            elements[size] = element;
            
            return size++;
        }

        // returns element that had to be moved to adjust
        Element remove(int index) {
            assert(index < size);

            // if last element
            if (index == size-1) {
                size--;
                return NullElement;
            }
            Element elementToMove = elements[size-1];
            elements[index] = elementToMove;

            for (int i = 0; i < buffers.size; i++) {
                auto componentSize = archetype.sizes[i];
                memcpy(buffers[i] + index * componentSize, buffers[i] + (size-1) * componentSize, componentSize);
            }

            size--;
            return elementToMove;
        }

        void destroy() {
            for (int i = 0; i < buffers.size; i++) {
                Free(buffers[i]);
                buffers.destroy();
            }
            Free(elements);
        }
    };

    using ArchetypeID = Sint16;
    static constexpr ArchetypeID NullArchetypeID = -1;

    My::Vec<ArchetypePool> pools;
    My::Vec<Element> unusedElements;
    My::HashMap<Signature, ArchetypeID, typename Signature::Hash> archetypes;
    ComponentInfoRef componentInfo;

    struct ElementData {
        Sint32 prototype;
        Signature signature;
        Uint32 version;
        Sint16 poolIndex;
        ArchetypeID archetype;
    };

    My::DenseSparseSet<ElementID, ElementData, 
        Uint16, MaxElementID> elementData;

    ArchetypalComponentManager() {}

    ArchetypalComponentManager(ComponentInfoRef componentInfo) : componentInfo(componentInfo) {
        auto nullPool = ArchetypePool(Archetype::Null());
        pools = My::Vec<ArchetypePool>(&nullPool, 1);
        archetypes = decltype(archetypes)::Empty();
        elementData = My::DenseSparseSet<ElementID, ElementData, 
            Uint16, MaxElementID>::WithCapacity(64);
        unusedElements = My::Vec<Element>::WithCapacity(1000);
        unusedElements.require(1000);
        for (int i = 0; i < 1000; i++) {
            unusedElements[i].id = i + 1;
            unusedElements[i].version = 0;
        }
    }
private:
    void* getElement(Element element) const {
        ElementData& data = elementData.lookup(element.id);
        if (element.version != data.version) {
            return nullptr;
        }

        if (data.archetype >= pools.size) {
            LogError("Invalid archetype %u", data.archetype);
            return nullptr;
        } 

        auto& archetype = pools[data.archetype];
        return archetype.get(data.index);
    }


public:

    Element newElement(Uint32 prototype) {
        Element element = unusedElements.popBack();
        ElementData* data = elementData.insert(element.id);
        data->version = ++element.version;
        data->archetype = 0;
        data->poolIndex = -1;
        data->signature = {0};
        data->prototype = prototype;
        return element;
    }

    void destroyElement(Element element) {
        const ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            // element isn't around anyway
            return;
        }

        ArchetypePool* pool = pools[data->archetype];
        pool->remove(element);

        elementData.remove(element.id);
        unusedElements.push(element);
    }

    const ElementData* getElementData(ElementID elementID) const {
        if (elementID == NullElement.id) {
            return nullptr;
        }
        return elementData.lookup(elementID);
    }

    Signature getElementSignature(Element element) const {
        if (element.id == NullElement.id) {
            return {0};
        }
        const ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            return {0};
        }

        return data->signature;
    }

    bool hasComponent(Element element, ComponentID component) const {
       if (element.id == NullElement.id) {
            return 0;
        }
        const ElementData* data = elementData.lookup(element.id);
        if (!data || (!element.version != data->version)) {
            return false;
        }

        return data->signature[component];
    }

    void* getComponent(Element element, ComponentID component) const {
        if (element.id == NullElement.id) {
            return nullptr;
        }
        const ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            return nullptr;
        }

        const auto& pool = pools[data->archetype];
        return pool.getComponent(component, data->poolIndex);
    }

    void* addComponent(Element element, ComponentID component, const void* initializationValue = nullptr) {
        if (element.id == NullElement.id) {
            return nullptr;
        }
        ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            return nullptr;
        }

        Signature oldSignature = data->signature;
        data->signature.set(component);

        auto newArchetypeID = getArchetypeID(data->signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(data->signature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        auto oldArchetypeID = data->archetype;
        if (newArchetypeID == oldArchetypeID) {
            // tried to add component that the element already has
            // just return the pointer to the component
            return newArchetype->getComponent(component, data->poolIndex);
        }

        int newElementIndex = newArchetype->addNew(element);
        
        if (oldArchetypeID > 0) {
            int oldElementIndex = data->poolIndex;
            ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
            for (int i = 0; i < oldArchetype->buffers.size; i++) {
                auto componentSize = oldArchetype->archetype.sizes[i];
                ComponentID transferComponent = oldArchetype->archetype.componentIDs[i];
                int newArchetypeIndex = newArchetype->archetype.getIndex(transferComponent);
                assert(newArchetypeIndex != -1);
                char* newComponentAddress = newArchetype->buffers[newArchetypeIndex] + componentSize * newElementIndex;
                char* oldComponentAddress = oldArchetype->buffers[i] + componentSize * oldElementIndex;
                memcpy(newComponentAddress, oldComponentAddress, componentSize);
            }

            Element movedElement = oldArchetype->remove(oldElementIndex);
            if (!movedElement.Null()) {
                ElementData* movedElementData = elementData.lookup(movedElement.id);
                assert(movedElementData);
                movedElementData->poolIndex = oldElementIndex;
            }
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newElementIndex;

        void* newComponentValue = newArchetype->getComponent(component, newElementIndex);
        if (initializationValue) {
            assert(newComponentValue);
            memcpy(newComponentValue, initializationValue, componentInfo.size(component));
        }

        return newComponentValue;
    }

    void removeComponent(Element element, ComponentID component) {
        if (element.id == NullElement.id) {
            return;
        }
        ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            return;
        }

        Signature oldSignature = data->signature;
        if (!oldSignature[component]) {
            // element didn't have the component. just do nothing
            return;
        }

        data->signature.set(component, 0);

        auto newArchetypeID = getArchetypeID(data->signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(data->signature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        int newElementIndex = newArchetype->addNew(element);

        auto oldArchetypeID = data->archetype;
        if (oldArchetypeID > 0) {
            int oldElementIndex = data->poolIndex;
            ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
            for (int i = 0; i < newArchetype->buffers.size; i++) {
                auto componentSize = newArchetype->archetype.sizes[i];
                ComponentID transferComponent = newArchetype->archetype.componentIDs[i];
                int oldArchetypeIndex = oldArchetype->archetype.getIndex(transferComponent);
                assert(oldArchetypeIndex != -1);
                char* oldComponentAddress = oldArchetype->buffers[oldArchetypeIndex] + componentSize * oldElementIndex;
                char* newComponentAddress = newArchetype->buffers[i] + componentSize * newElementIndex;
                memcpy(newComponentAddress, oldComponentAddress, componentSize);
            }

            Element movedElement = oldArchetype->remove(oldElementIndex);
            if (!movedElement.Null()) {
                ElementData* movedElementData = elementData.lookup(movedElement.id);
                assert(movedElementData);
                movedElementData->poolIndex = oldElementIndex;
            }
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newElementIndex;
    } 

private:
    // returns -1 if the archetype doesn't exist
    ArchetypeID getArchetypeID(Signature signature) const {
        auto* archetypeID = archetypes.lookup(signature);
        if (archetypeID) {
            return *archetypeID;
        }
        return NullArchetypeID;
    }

    ArchetypePool* getArchetypePool(ArchetypeID id) const {
        if (id >= pools.size) return nullptr;
        return &pools[id];
    }
public:

    ArchetypeID initArchetype(Signature signature) {
        assert(!archetypes.contains(signature));
        Archetype archetype = makeArchetype(signature, componentInfo);
 
        pools.push(archetype);
        archetypes.insert(signature, pools.size-1);
        return pools.size-1;
    }

    void destroy() {
        pools.destroy();
        unusedElements.destroy();
        archetypes.destroy();
        elementData.destroy();
    }
};

class LargeComponentList {
    using Set = My::DenseSparseSet<ComponentID, ComponentPtr, Uint16, maxComponentID>;
    Set components;
public:

    static LargeComponentList New() {
        LargeComponentList self;
        self.components = Set::Empty();
        return self;
    }

    void* get(ComponentID component) const {
        void** ptrToActualPtr = (void**)components.lookup(component);
        if (ptrToActualPtr) {
            return *ptrToActualPtr;
        }
        return nullptr;
    }

    // warning: slow and not too efficient
    bool has(ComponentID component) const {
        this->components.contains(component);
    }

    // storage must be stable, allocated memory to hold the component value
    void add(ComponentID component, void* storage) {
        components.insert(component, storage);
    }

    void remove(ComponentID component) {
        components.remove(component);
    }

    void destroy() {
        components.destroy();
    }
};

};

} // namespace GECS

#endif