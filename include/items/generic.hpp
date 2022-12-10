#ifndef GECS_INCLUDED
#define GECS_INCLUDED

#include "utils/ints.hpp"
#include "My/Bitset.hpp"
#include "My/HashMap.hpp"
#include "My/SparseSets.hpp"
#include "utils/common-macros.hpp"
#include <string>

#define GEN_IDS(type, list, firstID, elNumName) enum : type { FIRST(list) = firstID, list, elNumName };

namespace GECS {

using ComponentID = Sint16;
constexpr ComponentID NullComponentID = INT16_MIN;

struct ComponentInfo {
    size_t size;
    size_t alignment;
    std::string name;
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
        return components[component].name.c_str();
    }
};

template<size_t maxComponentID, class IDGetter>
struct Class {

using SignatureBase = My::Bitset<maxComponentID>;

struct Signature : SignatureBase {
    using SignatureBase::SignatureBase;
    
    constexpr Signature(SignatureBase bitset) : Signature(bitset) {
        
    }

    template<class C>
    constexpr bool getComponent() const {
        constexpr ComponentID id = getID<C>();
        return this->operator[](id);
    }

    template<class C>
    constexpr void setComponent(bool val) {
        constexpr ComponentID id = getID<C>();
        set(id, val);
    }
};
using ComponentPtr = void*;

template<class C>
constexpr ComponentID getRawID() {
    constexpr IDGetter idGetter;
    constexpr auto id = idGetter.get<C>();
    return id;
}

template<class C>
constexpr ComponentID getID() {
    constexpr auto raw = getRawID<C>();
    return raw ? raw : -raw;
}

template<class C>
constexpr bool isPrototype() {
    return getRawID<C>() < 0;
}

template<class C>
constexpr bool getPrototypeID() {
    static_assert(getID<C>() != NullComponentID, "Bad component!");
    static_assert(getID<C>() < 0, "Component isn't a prototype component!");
    return getID<C>();
}

template<class ...Cs>
constexpr Signature getSignature() {
    constexpr ComponentID ids[] = {getID<Cs>() ...};
    // sum component signatures
    auto result = Signature(0);
    for (size_t i = 0; i < sizeof...(Cs); i++) {
        if (ids[i] < result.size())
            result.set(ids[i]);
    }
    return result;
}

/*
using EntityID = Uint32;

struct Entity {
    Uint32 id;
    Uint16 version;
    Uint16 type;

    Signature signature;
};

template<class Base>
struct EntityManager {
    // Base Interface:
    // void*    get(EntityID, ComponentID)
    // void* getProto()
    // void     add(EntityID, ComponentID)
    // void  remove(EntityID, ComponentID)

    Base& base() {
        return *static_cast<Base*>(this);
    }
    const Base& base() const {
        return *static_cast<const Base*>(this);
    }

    void* get(Entity entity, ComponentID component) {
        base().get(entity.id, component);
    }

    template<class C>
    C* get(Entity entity) {
        if (!isPrototype<C>()) {
            constexpr auto id = getID<C>();
            return static_cast<C*>(get(entity.id, id));
        } else {
            constexpr auto id = getPrototypeID<C>();
            return static_cast<C*>(get(entity.id, id));
        }
    }

    template<class ...Cs>
    bool has(Entity entity) const {
        return entity.signature & getSignature<Cs...>();
    }
};

inline void testefjoais() {
    (void)sizeof(Entity);
}
*/

template <size_t I, typename Tuple>
constexpr size_t element_offset() {
    using element_t = std::tuple_element_t<I, Tuple>;
    static_assert(!std::is_reference_v<element_t>);
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
    static_assert(!std::is_reference_v<element_t>);
    auto* p = std::addressof(std::get<T>(t));
    t.~Tuple();
    std::size_t off = 0;
    for (std::size_t i = 0;; ++i) {
        if (static_cast<void*>(a + i) == p) return i;
    }
}

struct ArchetypalComponentManager {

    ArchetypalComponentManager() {}
 
    struct Archetype {
        using ComponentOffsetSet = My::DenseSparseSet<ComponentID, uint16_t, maxComponentID>;
        ComponentOffsetSet componentOffsets;
        uint32_t totalSize;
        int32_t numComponents;
        Signature signature;
        uint16_t* sizes;

        static Archetype Null() {
            return {ComponentOffsetSet::New(), 0, 0, 0, nullptr};
        }

        uint16_t getOffset(ComponentID component) const {
            uint16_t* offset = componentOffsets.lookup(component);
            assert(offset && "Invalid component!");
            return *offset;
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
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, maxComponentID>;
        constexpr auto sizes = {sizeof(Components) ...};
        constexpr auto offsets = {element_offset<Components, Tuple>()};
        for (int i = 0; i < count; i++) {
            archetype.sizes[i] = sizes[i];
            archetype.componentOffsets.insert(ids[i], offsets[i]);
        }
    }

    static Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo) {
        Archetype archetype;
        auto count = sizeof...(Components);
        archetype.signature = getSignature<Components...>();
        archetype.numComponents = count;
        archetype.totalSize = 0;
        archetype.sizes = Alloc<uint16_t>(count);
        archetype.componentOffsets = My::DenseSparseSet<ComponentID, uint16_t, MaxComponentID>;
        auto sizes = {sizeof(Components) ...};
        uint16_t offset = 0;
        int index = 0;
        signature.forEachSet([&](auto componentID){
            auto componentSize = (uint16_t)componentTypeInfo[componentID].size;
            archetype.sizes[index] = componentSize;
            archetype.componentOffsets.insert(componentID, offset);
            offset += componentSize;
            archetype.totalSize += componentSize;
            index++;
        });
    }

    struct ArchetypePool {
        void* data = nullptr;
        int elementCount = 0;
        int elementCapacity = 0;
        Archetype archetype;

        ArchetypePool(const Archetype& archetype) : archetype(archetype) {
            
        }

        // index must be in bounds
        void* get(uint32_t index) const {
            assert(index < elementSize);
            return (char*)data + index * (uint32_t)archetype.totalSize;
        }

        void destroy() {
            Free(data);
        }
    };

    using ArchetypeID = int32_t;
    constexpr ArchetypeID NullArchetypeID = -1;

    static constexpr uint32_t MaxNumArchetypes = 1 << 10; // 10 bits

    struct ElementAddress {
        uint32_t archetype:10;
        uint32_t index:22;

        constexpr static ElementAddress Null = {0};
    };

    struct ElementList {
        ElementAddress elements;
        uint32_t count;
    };

    My::Vec<ArchetypePool> pools;
    My::HashMap<Signature, ArchetypeID> archetypes;
    ComponentInfoRef componentInfo;

    ArchetypalComponentManager(ComponentInfoRef componentInfo) : componentInfo(componentInfo) {
        auto nullPool = ArchetypePool(Archetype::Null());
        pools = My::Vec<ArchetypePool>(&nullPool, 1);
    }

    ElementAddress 

    void* getElement(ElementAddress element) {
        if (element.archetype >= MaxNumArchetypes) return nullptr;
        auto& archetype = pools[element.archetype];
        return archetype.get(element.index);
    }

    void* getComponent(ElementAddress element, ComponentID component) {
        if (element.archetype >= MaxNumArchetypes) {
            auto& archetype = pools[element.archetype];
            if (archetype.archetype.signature[component]) {
                void* element = pool.get(element.index);
                auto componentOffset = archetype.getOffset(component);
                return (char*)element + (char*)componentOffset;
            }
        }
        return nullptr;
    }

private:
    // returns -1 if the archetype doesn't exist
    ArchetypeID getArchetypeID(Signature signature) const {
        auto* archetypeID = archetypes.lookup(signature);
        if (archetypeID) {
            return *archetypeID;
        }
        return -1;
    }
public:

    ArchetypeID initArchetype(Signature signature) {
        assert(!archetypes.contains(signature));
        Archetype archetype = makeArchetype(signature, componentInfo);
        if (pools.size >= MaxNumArchetypes) {
            return -1;
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
        /*
        ElementList elementList;
        elementList.elements.archetype = archetypeID;
        elementList.elements.index = backIndex;
        elementList.count = count;
        return elementList;
        */
        return ElementList{
            .elements = ElementAddress{
                .archetype = archetypeID,
                .index = backIndex
            },
            .count = count;
        }
    }
};

/*
struct ItemComponentManager {
    //using ComponentIndex = Sint32;
    //My::Vec< My::Vector::Generic::GenericVec > pools;
    //My::DenseSparseSet<ItemInstanceID, ComponentIndex, 100000> set;

    struct ArchetypePool {
        void* components;
        Archetype archetype;
    };

    My::Vec<ArchetypePool*> pools;
    

    void makeNewArchetype(Archetype archetype) {
        ArchetypePool pool;
        pool.archetype = archetype;
        pool.components = nullptr;
    }

    template<class C>
    void initComponentType() {
        constexpr auto id = getItcID<C>();
        if (validItcID(id)) {
            if (pools.size < id) {
                pools.resize(id);
            }
        }
    }

    

    void* get(ItemInstanceID item, ItemComponentID component) {
        auto& pool = pools[component];
        auto* lookupResult = set.lookup(item);
        if (lookupResult)
            return pool.get(*lookupResult);
        return nullptr;
    }
};
*/

struct SmallComponentList {
    using ComponentVec = My::VecTuple<ItemECS::ComponentID, ItemECS::ComponentPtr>;

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

class LargeComponentList {
    using Set = My::DenseSparseSet<ComponentID, ComponentPtr, maxComponentID>;
    Set components;
public:

    static LargeComponentList New() {
        components = Set::New()
    }

    void* get(ComponentID component) const {
        return components.lookup(component);
    }

    // warning: slow and not too efficient
    bool has(ComponentID component) const {
        this->components.contains(component);
    }

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