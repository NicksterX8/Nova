#ifndef ECS_ARCHETYPE_POOL_INCLUDED
#define ECS_ARCHETYPE_POOL_INCLUDED

#include <array>
#include "ADT/ArrayRef.hpp"
#include "ADT/SmallVector.hpp"
#include "My/SparseSets.hpp"
#include "My/Vec.hpp"
#include "utils/ints.hpp"
#include "Entity.hpp"
#include "Signature.hpp"
#include "ComponentInfo.hpp"
#include "memory/Allocator.hpp"

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

namespace ECS {

struct Archetype {
    Sint8 componentIndices[MaxComponentIDs];
    int32_t numComponents;
    Signature signature;
    uint16_t* sizes;
    uint16_t* alignments;
    ComponentID* componentIDs;
    int32_t sumSize; // size of all components added

    static Archetype Null() {
        return {{-1}, 0, 0, nullptr, nullptr, nullptr, 0};
    }

    Sint8 getIndex(ComponentID component) const {
        if (component < 0 || component >= MaxComponentIDs) return -1;
        return componentIndices[component];
    }

    Uint16 getSize(ComponentID component) const {
        return sizes[componentIndices[component]];
    }
};

Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo);

struct ArchetypePool {
    int size;
    int capacity;
    Entity* entities; // contained entities
    char* buffer;
    My::Vec<int> bufferOffsets; // make SmallVector? there could be like 15 components and then its just a waste of space
    Archetype archetype;

    ArchetypePool(const Archetype& archetype);

    bool empty() const {
        return size == 0;
    }

    Signature signature() const {
        return archetype.signature;
    }

    int numBuffers() const {
        return bufferOffsets.size;
    }

    char* getComponentArray(ComponentID componentType) const {
        auto bufferIndex = archetype.getIndex(componentType);
        if (bufferIndex == -1) return nullptr; // component array not in pool
        return getBuffer(bufferIndex);
    }

    char* getBuffer(int bufferIndex) const {
        assert(buffer && "Null archetype pool!");
        assert(bufferIndex >= 0 && bufferIndex < numBuffers() && "Buffer index out of range!");
        return buffer + bufferOffsets[bufferIndex];
    }

    char* getComponentByIndex(int bufferIndex, int componentIndex) const {
        char* buf = getBuffer(bufferIndex);
        if (buf) {
            return buf + archetype.sizes[bufferIndex] * componentIndex;
        }
        return nullptr;
    }

    // index must be in bounds
    void* getComponent(ComponentID component, int index) const {
        assert(index < size && index >= 0);
        auto bufferIndex = archetype.getIndex(component);
        if (bufferIndex < 0) return nullptr;
        return getComponentByIndex(bufferIndex, index);
    }

    // returns index where entity is stored
    int addNew(Entity entity);

    void copyIndex(int dstIndex, int srcIndex) {
        for (int i = 0; i < archetype.numComponents; i++) {
            auto componentSize = archetype.sizes[i];
            char* componentBuffer = buffer + bufferOffsets[i];
            memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
        }
        entities[dstIndex] = entities[srcIndex];
    }

    // equivalent to 
    // copyIndex(dstIndex, middleIndex)
    // copyIndex(middleIndex, srcIndex)
    void doubleCopyIndex(int dstIndex, int middleIndex, int srcIndex);

    // returns entities moved in movedEntity0 and movedEntity1. If less than 2 entities were moved, they will be set to NullEntity and newIndex will be undefined
    // if movedEntity0 is NullEntity, movedEntity1 will also be
    void remove(int index, Entity* movedEntity);

    void clear() {
        size = 0;
    }

    void destroy() {
        Free(buffer);
        bufferOffsets.destroy();
        Free(entities);
    }
};

} // namespace ECS

#endif