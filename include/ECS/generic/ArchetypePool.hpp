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
        using ComponentOffsetSet = My::DenseSparseSet<ComponentID, uint16_t, uint16_t, maxComponentID>;
        ComponentOffsetSet componentOffsets;
        uint32_t totalSize;
        int32_t numComponents;
        Signature signature;
        uint16_t* sizes;

        static Archetype Null() {
            return {ComponentOffsetSet::Empty(), 0, 0, 0, nullptr};
        }

        // returns pointer to offset value on success,
        // returns nullptr on error - if component isn't in the archetype
        const uint16_t* getOffset(ComponentID component) const {
            return componentOffsets.lookup(component);
        }
    };

    template<class... Components>
    static Archetype makeArchetype() {
        using Tuple = std::tuple<Components...>();

        Archetype archetype;
        constexpr auto count = sizeof...(Components);
        archetype.signature = getSignature<Components...>();
        archetype.numComponents = count;
        archetype.totalSize = sizeof(Tuple);
        archetype.sizes = Alloc<uint16_t>(count);
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, uint16_t, maxComponentID>::Empty();
        constexpr auto sizes   = {sizeof(Components) ...};
        constexpr auto ids     = {getID<Components>() ...};
        constexpr auto offsets = {element_offset<Components, Tuple>() ...};
        for (int i = 0; i < count; i++) {
            archetype.sizes[i] = sizes[i];
            archetype.componentOffsets.insert(ids[i], offsets[i]);
        }
    }

    static Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo) {
        Archetype archetype;
        const auto count = signature.count();
        archetype.signature = signature;
        archetype.numComponents = count;
        archetype.totalSize = 0;
        archetype.sizes = Alloc<uint16_t>(count);
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, uint16_t, maxComponentID>::Empty();
        uint16_t offset = 0;
        int index = 0;
        signature.forEachSet([&](auto componentID){
            auto componentSize = (uint16_t)componentTypeInfo.size(componentID);
            archetype.componentOffsets.insert(componentID, offset);
            offset += componentSize;
            archetype.totalSize += componentSize;
            index++;
        });
        return archetype;
    }

    static constexpr size_t MaxElementID = (1 << 14) - 1;

    struct ArchetypePool {
        My::Vec<Element> elements; // contained elements
        My::Vec<char> buffer;
        Archetype archetype;
        
        ArchetypePool(const Archetype& archetype) : archetype(archetype) {
            elements = My::Vec<Element>::Empty();
            buffer = My::Vec<char>::WithCapacity(archetype.totalSize);
        }

        // index must be in bounds
        void* getComponents(int index) const {
            //assert(index < elementCount);
            //return (char*)data + index * (uint32_t)archetype.totalSize;
            return buffer.data + index * archetype.totalSize;
        }

        // returns index where element is stored
        int addNew(Element element) {
            elements.push(element);
            buffer.require(archetype.totalSize);
            return elements.size-1;
        }

        void remove(int index) {
            // if last element
            if (index == elements.size-1) {
                elements.size--;
                return;
            }
            elements[index] = elements[elements.size-1];

            void* newSpace = getComponents(index);
            void* oldSpace = getComponents(elements.size-1);
            memcpy(newSpace, oldSpace, archetype.totalSize);

            elements.size--;
        }

        void destroy() {
            elements.destroy();
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
        const Uint16* offset = pool.archetype.getOffset(component);
        if (!offset) return nullptr;
        return (char*)pool.getComponents(data->poolIndex) + *offset;
    }

    void* addComponent(Element element, ComponentID component) {
        if (element.id == NullElement.id) {
            return nullptr;
        }
        ElementData* data = elementData.lookup(element.id);
        if (!data || (element.version != data->version)) {
            return nullptr;
        }

        if (data->archetype >= pools.size) {
            LogError("Element archetype %d out of range!", data->archetype);
            return nullptr;
        }

        Signature oldSignature = data->signature;
        data->signature.set(component);

        auto newArchetypeID = getArchetypeID(data->signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(data->signature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        int newElementIndex = newArchetype->addNew(element);
        void* newElementValue = newArchetype->getComponents(newElementIndex);
        assert(newElementValue && "couldn't make new element index!");
        
        if (data->archetype > 0) {
            auto oldArchetype = getArchetypePool(data->archetype);
            void* oldElementValue = oldArchetype->getComponents(data->poolIndex);
            if (oldElementValue) {
                // copy element archetype data - if entity data is stored adjacent to component data
                //memcpy(newElementValue, oldElementValue, sizeof(ElementArchetypeData));
                oldSignature.forEachSet([&](ComponentID componentId){
                    const uint16_t* oldComponentOffset = oldArchetype->archetype.getOffset(componentId);
                    assert(oldComponentOffset);
                    void* oldComponentValue = (char*)oldElementValue + *oldComponentOffset;
                    const uint16_t* newComponentOffset = newArchetype->archetype.getOffset(componentId);
                    assert(newComponentOffset);
                    void* newComponentValue = (char*)newElementValue + *newComponentOffset;

                    size_t componentSize = componentInfo.size(componentId);
                    memcpy(newComponentValue, oldComponentValue, componentSize);
                });
            }
            oldArchetype->remove(data->poolIndex);
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newElementIndex;

        const Uint16* addedComponentOffset = newArchetype->archetype.getOffset(component);
        assert(addedComponentOffset);
        void* addedComponent = (char*)newElementValue + *addedComponentOffset;

        return addedComponent;
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

/*
struct SmallComponentList {
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