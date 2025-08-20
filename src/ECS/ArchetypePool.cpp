#include "ECS/ArchetypePool.hpp"

namespace ECS {

ArchetypePool::ArchetypePool(Signature signature, PoolAllocator* metaAllocator) {
    _signature = signature;
    int componentCount = signature.count();
    _numComponents = componentCount;
    entities = nullptr;
    arrays = metaAllocator->allocate<char*>(componentCount);
    memset(arrays, 0, componentCount * sizeof(char*));
    componentIDs = metaAllocator->allocate<ComponentID>(componentCount);

    auto bits = signature.bits[0];
    auto numComponentsInWord = llvm::countPopulation(bits);
    numComponentsInOrBeforeWord[0] = numComponentsInWord;
    for (int i = 1; i < Signature::nInts; i++) {
        auto bits = signature.bits[i];
        auto numComponentsInWord = llvm::countPopulation(bits);
        numComponentsInOrBeforeWord[i] = numComponentsInWord + numComponentsInOrBeforeWord[i - 1];
    }

    int i = 0;
    signature.forEachSet([&](ComponentID component){
        componentIDs[i++] = component;
    });
    size = 0;
    capacity = 0;
}

int ArchetypePool::getArrayNumberFromSignature(ComponentID component) const {
    static_assert(sizeof(Signature::IntegerType) == sizeof(uint64_t), "");
    auto word = component / Signature::IntegerBits;
    auto wordIndex = component % Signature::IntegerBits;
    uint64_t bits = _signature.bits[word];
    uint64_t componentBit = ((uint64_t)1 << wordIndex);
    if (!(bits & componentBit)) {
        // archetype does not have component
        return -1;
    }
    uint64_t lowerMask = componentBit - 1;
    int index = llvm::countPopulation(bits & lowerMask);
    // now to get index for signature in its entirety add number of components before this word
    if constexpr (Signature::nInts > 1) {
        if (word > 1) index += numComponentsInOrBeforeWord[word - 1];
    }
    return index;
}

int ArchetypePool::addNew(int count, const Entity* newEntities, ArchetypeAllocator* allocator, const Sint32* componentSizes) {
    int nNewEntities = count;
    if (size + nNewEntities > capacity) {
        int newCapacity = (capacity * 2 >= size + nNewEntities) ? capacity * 2 : size + nNewEntities;
        entities = allocator->reallocate(entities, (size_t)capacity, (size_t)newCapacity);
        for (int i = 0; i < _numComponents; i++) {
            ComponentID componentID = componentIDs[i];
            auto componentSize = componentSizes[componentID];
            arrays[i] = (char*)allocator->reallocate(arrays[i], capacity * componentSize, newCapacity * componentSize, alignof(max_align_t));
        }
        capacity = newCapacity;
    }

    int startIndex = size;
    
    if (newEntities)
        memcpy(entities + size, newEntities, nNewEntities * sizeof(Entity));

    size += nNewEntities;
    return startIndex;
}

// void ArchetypePool::doubleCopyIndex(int dstIndex, int middleIndex, int srcIndex) {
//     for (int i = 0; i < archetype.numComponents; i++) {
//         auto componentSize = archetype.sizes[i];
//         char* componentBuffer = buffer + bufferOffsets[i];

//         // copy middle to dst
//         memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + middleIndex * componentSize, componentSize);
//         // then copy src to middle
//         memcpy(componentBuffer + middleIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
//     }
//     entities[dstIndex] = entities[middleIndex];
//     entities[middleIndex] = entities[srcIndex];
// }

void ArchetypePool::remove(int index, Entity* movedEntity, const Sint32* componentSizes) {
    assert(index < size);

    const int lastIndex = size - 1;
    if (index < size-1) {
        // much simpler
        *movedEntity = entities[lastIndex];
        copyIndex(index, lastIndex, componentSizes);
    } else {
        // removing very last entity.
        // doesn't matter whether it's dirty or not,
        // no swapping to do
        *movedEntity = NullEntity;
    }

    size--;
}

// void ArchetypePool::remove(ArrayRef<int> indices, EntityLoc* entityLocations, const Sint32* componentSizes) {
//     assert(indices.size() <= size);

//     const int lastIndex = size - 1;

//     for (int i = 0; i < _numComponents; i++) {
//         int componentID = componentIDs[i];
//         char* componentBuffer = arrays[i];
//         auto componentSize = componentSizes[componentID];
//         for (int j = 0; j < indices.size(); j++) {
//             int dstIndex = indices[j];
//             int srcIndex = size - 1 - j;
//             if (dstIndex == srcIndex) continue; // happens when we are removing back most entity. necessary cause memcpy(ptr, ptr) is ub
//             memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
//         }
//     }

//     for (int i = 0; i < indices.size(); i++) {
//         int dstIndex = indices[i];
//         int srcIndex = size - 1 - i;
//         if (dstIndex == srcIndex) continue;
//         entities[dstIndex] = entities[srcIndex];
//         entityLocations[entities[dstIndex].id]
//     }

//     size -= indices.size();
// }

}