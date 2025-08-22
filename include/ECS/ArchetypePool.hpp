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
    struct ComponentArray {
        ComponentID componentType;
        char* data;
    };
    ComponentArray* arrays; // arrays for components
    Sint32 _numComponents; // any component in the signature (signature.count())
    EntityIndex size;
    EntityIndex capacity;
    Entity* entities; // contained entities
    Signature _signature; // maybe make this SoA to make queries faster
    uint8_t numComponentsInOrBeforeWord[Signature::nInts];
    
    ArchetypePool(Signature signature, const Sint32* componentSizes, PoolAllocator* metaAllocator);

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
            if (arrays[i].componentType == component) return i;
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

    int numComponentArrays() const {
        return this->_numComponents;
    }

    char* getComponentArray(ComponentID componentType) const {
        int arrayNum = getArrayNumber(componentType);
        if (arrayNum == -1) return nullptr;
        else return arrays[arrayNum].data;
    }

    char* getArray(int arrayIndex) const {
        DASSERT(arrayIndex < _numComponents);
        return arrays[arrayIndex].data;
    }

    char* getComponentByIndex(int arrayIndex, int componentIndex, int componentSize) const {
        char* array = arrays[arrayIndex].data;
        return array + componentIndex * componentSize;
    }

    // index must be in bounds
    char* getComponent(ComponentID component, int index, int componentSize) const {
        DASSERT(index < size && index >= 0);
        int arrayNum = getArrayNumber(component);
        if (arrayNum == -1) return nullptr;
        char* array = arrays[arrayNum].data;
        return array + index * componentSize;
    }

    // returns index where entity is stored
    // @param newEntities may be null and entities will be left uninitialized, they must be initialized after calling this!
    int addNew(int count, const Entity* newEntities, ArchetypeAllocator* allocator, const Sint32* componentSizes);

    void copyIndex(int dstIndex, int srcIndex, const Sint32* componentSizes) {
        for (int i = 0; i < _numComponents; i++) {
            int componentID = arrays[i].componentType;
            char* componentBuffer = arrays[i].data;
            auto componentSize = componentSizes[componentID];
            memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
        }
        entities[dstIndex] = entities[srcIndex];
    }

    void remove(int index, Entity* movedEntity, const Sint32* componentSizes);

    // void remove(ArrayRef<int> indices, SmallVectorImpl<EntityID>* movedEntities, const Sint32* componentSizes);

    void clear() {
        size = 0;
    }

    void destroy(PoolAllocator* poolAllocator, ArchetypeAllocator* archetypeAllocator, const Sint32* componentSizes) {
        archetypeAllocator->deallocate(entities, capacity);
        for (int i = 0; i < _numComponents; i++) {
            ComponentID component = arrays[i].componentType;
            archetypeAllocator->deallocate(arrays[i].data, capacity * componentSizes[component], alignof(std::max_align_t));
        }
        poolAllocator->deallocate(arrays, _numComponents);
    }
};

} // namespace ECS

#endif