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
#include "memory/ArenaAllocator.hpp"

namespace ECS {

using ArchetypeAllocator = Mallocator;
using PoolAllocator = Mallocator;

struct ArchetypePool {
    using EntityIndex = Sint16;

    Signature _signature; // maybe make this SoA to make queries faster
    uint8_t numComponentsInOrBeforeWord[Signature::nInts];

    int _numComponents;
    int size;
    int capacity;
    Entity* entities; // contained entities
    char** arrays; // arrays for components
    ComponentID* componentIDs;
    
    ArchetypePool(Signature signature, PoolAllocator* metaAllocator);

    bool null() const {
        return _numComponents == 0;
    }

    bool empty() const {
        return size == 0;
    }

    Signature signature() const {
        return _signature;
    }

    int getArrayNumberFromSignature(ComponentID component) const;

    int getArrayNumberLinear(ComponentID component) const {
        for (int i = 0; i < _numComponents; i++) {
            if (componentIDs[i] == component) return i;
        }
        return -1;
    }

    // component must be in archetype!
    int getArrayNumber(ComponentID component) const {
        if (_numComponents < 4 * Signature::nInts) {
            return getArrayNumberLinear(component);
        } else {
            return getArrayNumberFromSignature(component);
        }
    }

    int numComponents() const {
        return _numComponents;
    }

    char* getComponentArray(ComponentID componentType) const {
        int arrayNum = getArrayNumber(componentType);
        if (arrayNum == -1) return nullptr;
        else return arrays[arrayNum];
    }

    char* getArray(int arrayIndex) const {
        DASSERT(arrayIndex < _numComponents);
        return arrays[arrayIndex];
    }

    char* getComponentByIndex(int arrayIndex, int componentIndex, int componentSize) const {
        char* array = arrays[arrayIndex];
        return array + componentIndex * componentSize;
    }

    // index must be in bounds
    char* getComponent(ComponentID component, int index, int componentSize) const {
        DASSERT(index < size && index >= 0);
        int arrayNum = getArrayNumber(component);
        if (arrayNum == -1) return nullptr;
        char* array = arrays[arrayNum];
        return array + index * componentSize;
    }

    // returns index where entity is stored
    // @param newEntities may be null and entities will be left uninitialized, they must be initialized after calling this!
    int addNew(int count, const Entity* newEntities, ArchetypeAllocator* allocator, const Sint32* componentSizes);

    void copyIndex(int dstIndex, int srcIndex, const Sint32* componentSizes) {
        for (int i = 0; i < _numComponents; i++) {
            int componentID = componentIDs[i];
            char* componentBuffer = arrays[i];
            auto componentSize = componentSizes[componentID];
            memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
        }
        entities[dstIndex] = entities[srcIndex];
    }

    void remove(int index, Entity* movedEntity, const Sint32* componentSizes);

    void remove(ArrayRef<int> indices, SmallVectorImpl<EntityID>* movedEntities, const Sint32* componentSizes);

    void clear() {
        size = 0;
    }

    void destroy(PoolAllocator* poolAllocator, ArchetypeAllocator* archetypeAllocator, const Sint32* componentSizes) {
        archetypeAllocator->deallocate(entities, capacity);
        for (int i = 0; i < _numComponents; i++) {
            ComponentID component = componentIDs[i];
            archetypeAllocator->deallocate(arrays[i], capacity * componentSizes[component], alignof(std::max_align_t));
        }
        poolAllocator->deallocate(arrays, _numComponents);
        poolAllocator->deallocate(componentIDs, _numComponents);
    }
};

} // namespace ECS

#endif