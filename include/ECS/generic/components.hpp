#ifndef GECS_INCLUDED
#define GECS_INCLUDED

#include "utils/ints.hpp"
#include "My/Bitset.hpp"
#include "My/HashMap.hpp"
#include "My/SparseSets.hpp"
#include "My/Vec.hpp"
#include "utils/common-macros.hpp"
#include <string>
#include "global.hpp"


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

#define GEN_IDS(name, type, list, elNumName) enum name : type { list, elNumName };
//#define GEN_IDS_STARTING(type, list, elNumName, firstName, firstID) enum : type { firstName = firstID, list, elNumName };

namespace GECS {

using ComponentID = Sint16;
constexpr ComponentID NullComponentID = INT16_MIN;
using ComponentPtr = void*;

struct ComponentInfo {
    size_t size = 0;
    size_t alignment = 0;
    const char* name = "null";
};

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

template<class Component>
static constexpr ComponentInfo getComponentInfo() {
    return {sizeof(Component), alignof(Component), Component::NAME};
}

template<class... Components>
static constexpr std::array<ComponentInfo, sizeof...(Components)> getComponentInfoList() {
    return {getComponentInfo<Components>() ...};
}

struct DefaultIDGetter {
    template<class C>
    static constexpr ComponentID get() {
        return C::ID;
    }
};

template<size_t maxComponentID, class IDGetter = DefaultIDGetter>
struct Signature : My::Bitset<maxComponentID> {
    private: using Base = My::Bitset<maxComponentID>; public:
    using Self = Signature<maxComponentID>;

    constexpr Signature() {}

    /*
    constexpr Signature(const Self& self) : Base(self) {

    }
    */

    constexpr Signature(typename Base::IntegerType startValue) : Base(startValue) {

    }
    
    /*
    constexpr Signature(Base bitset) : My::Bitset<maxComponentID>(bitset) {
        
    }
    */

    template<class C>
    constexpr bool getComponent() const {
        constexpr ComponentID id = IDGetter::template get<C>();
        return this->operator[](id);
    }

    template<class C>
    constexpr void setComponent(bool val) {
        constexpr ComponentID id = IDGetter::template get<C>();
        My::Bitset<maxComponentID>::set(id, val);
    }

    struct Hash {
        size_t operator()(Self self) const {
            //TODO: OMPTIMIZE improve hash
            // intsPerHash will always be 1 or greater as IntegerT cannot be larger than size_t
            constexpr size_t intsPerHash = sizeof(size_t) / Self::IntegerSize;
            size_t hash = 0;
            for (int i = 0; i < self.nInts; i++) {
                for (int j = 0; j < intsPerHash; j++) {
                    hash ^= (size_t)self.bits[i] << j * Self::IntegerSize * CHAR_BIT;
                }
            }
            return hash;
        }
    };
};


/*
Example id getter:
struct IDGetter {
    template<class C>
    static constexpr ComponentID get() {
        return X;
    }
};
*/

#define GECS_ARCHETYPE_ELEMENT_ARCHETYPE_BITS 10
#define GECS_ARCHETYPE_ELEMENT_INDEX_BITS 22
using ArchetypeElementIndexType = uint32_t;
#define GECS_MAX_ARCHETYPE_POOL_SIZE ((size_t)1 << (size_t)MIN(sizeof(ArchetypeElementIndexType) * CHAR_BIT, GECS_ARCHETYPE_ELEMENT_INDEX_BITS))

struct ArchetypeElementAddress {
    uint32_t archetype:GECS_ARCHETYPE_ELEMENT_ARCHETYPE_BITS;
    uint32_t index:GECS_ARCHETYPE_ELEMENT_INDEX_BITS;
};
static_assert(sizeof(ArchetypeElementAddress) == 4, "Oversized element address struct. Check bits used.");

constexpr static ArchetypeElementAddress NullElementAddress = { .archetype = 0, .index = 0 };

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

struct ArchetypalComponentManager {
    struct Archetype {
        using ComponentOffsetSet = My::DenseSparseSet<ComponentID, uint16_t, maxComponentID>;
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
        uint16_t* getOffset(ComponentID component) const {
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
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, maxComponentID>::Empty();
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
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, maxComponentID>::Empty();
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

    struct ArchetypePool {
        My::GenericDenseSparseSet<ArchetypeElementIndexType, GECS_MAX_ARCHETYPE_POOL_SIZE> entities;
        My::Vec<ArchetypeElementIndexType> freeIndices;
        Archetype archetype;

        ArchetypePool(const Archetype& archetype) : archetype(archetype) {
            entities = decltype(entities)::New(archetype.totalSize);
            freeIndices = My::Vec<ArchetypeElementIndexType>::WithCapacity(100);
            for (int i = 0; i < 100; i++) {
                freeIndices.push(i);
            }
        }

        // index must be in bounds
        void* get(ArchetypeElementIndexType index) const {
            //assert(index < elementCount);
            //return (char*)data + index * (uint32_t)archetype.totalSize;
            return entities.lookup(index);
        }

        ArchetypeElementIndexType getNew() {
            auto index = freeIndices.back();
            freeIndices.pop();

            entities.insert(index);
            return index;
        }

        void remove(ArchetypeElementIndexType index) {
            entities.remove(index);
            freeIndices.push(index);
        }

        void destroy() {
            entities.destroy();
        }
    };

    using ArchetypeID = uint16_t;
    static constexpr ArchetypeID NullArchetypeID = (1 << GECS_ARCHETYPE_ELEMENT_ARCHETYPE_BITS) - 1;

    static constexpr uint32_t MaxNumArchetypes = (1 << GECS_ARCHETYPE_ELEMENT_ARCHETYPE_BITS) - 1; // 10 bits address space - 1 for null archetype

    using ElementAddress = ArchetypeElementAddress;
    static constexpr auto NullAddress = NullElementAddress;

    struct ElementList {
        ElementAddress elements;
        uint32_t count;
    };

    My::Vec<ArchetypePool> pools;
    My::HashMap<Signature, ArchetypeID, typename Signature::Hash> archetypes;
    ComponentInfoRef componentInfo;

    ArchetypalComponentManager() {}

    ArchetypalComponentManager(ComponentInfoRef componentInfo) : componentInfo(componentInfo) {
        auto nullPool = ArchetypePool(Archetype::Null());
        pools = My::Vec<ArchetypePool>(&nullPool, 1);
        archetypes = decltype(archetypes)::Empty();
    }
private:
    void* getElement(ElementAddress element) const {
        if (element.archetype >= MaxNumArchetypes) return nullptr;
        auto& archetype = pools[element.archetype];
        return archetype.get(element.index);
    }
public:
    Signature getElementSignature(ElementAddress element) const {
        const auto& archetype = pools[element.archetype].archetype;
        return archetype.signature;
    }

    void* getComponent(ElementAddress element, ComponentID component) const {
        assert(element.archetype < MaxNumArchetypes && "Invalid element archetype!");
        const auto& pool = pools[element.archetype];
        if (pool.archetype.signature[component]) {
            void* elementPtr = pool.get(element.index);
            uint16_t* componentOffset = pool.archetype.getOffset(component);
            if (componentOffset) {
                return (char*)elementPtr + *componentOffset;
            }
        }
        return nullptr;
    }

    bool addComponent(ElementAddress* element, ComponentID component) {
        assert(element->archetype < MaxNumArchetypes && "Invalid element archetype!");

        if (element->archetype >= pools.size) {
            LogError("Element archetype %d out of range!", element->archetype);
            return false;
        }
        auto& pool = pools[element->archetype];

        Signature oldSignature = pool.archetype.signature;
        Signature newSignature = oldSignature; newSignature.set(component);

        auto oldArchetypeID = getArchetypeID(oldSignature);
        auto newArchetypeID = getArchetypeID(newSignature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(newSignature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        ElementAddress newElement = {
            newArchetypeID,
            newArchetype->getNew()
        };
        if (oldArchetypeID != NullArchetypeID) {
            auto oldArchetype = getArchetypePool(oldArchetypeID);
            void* oldElementValue = oldArchetype->get(element->index);
            void* newElementValue = newArchetype->get(newElement.index);
            assert(newElementValue && "couldn't make new element index!");
            if (oldElementValue) {
                oldSignature.forEachSet([&](ComponentID componentId){
                    uint16_t* oldComponentOffset = oldArchetype->archetype.getOffset(componentId);
                    assert(oldComponentOffset);
                    void* oldComponentValue = (char*)oldElementValue + *oldComponentOffset;
                    uint16_t* newComponentOffset = newArchetype->archetype.getOffset(componentId);
                    assert(newComponentOffset);
                    void* newComponentValue = (char*)newElementValue + *newComponentOffset;

                    size_t componentSize = componentInfo.size(componentId);
                    memcpy(newComponentValue, oldComponentValue, componentSize);
                });
            }
            oldArchetype->remove(element->index);
        }

        *element = newElement;

        return true;
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
        if (pools.size >= MaxNumArchetypes) {
            return NullArchetypeID;
        }
        pools.push(archetype);
        archetypes.insert(signature, pools.size-1);
        return pools.size-1;
    }

    ElementList newElements(Signature signature, int count) {
        auto archetypeID = getArchetypeID(signature);
        if (archetypeID == NullArchetypeID) {
            archetypeID = initArchetype(signature);
        }
        auto& pool = pools[archetypeID];
        uint32_t backIndex = pool.size;
        pool.reserve(pool.elementCount + count);
        pool.elementCount += count;
        return ElementList{
            .elements = ElementAddress{
                .archetype = archetypeID,
                .index = backIndex
            },
            .count = count
        };
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
    using Set = My::DenseSparseSet<ComponentID, ComponentPtr, maxComponentID>;
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