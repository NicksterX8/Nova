#include "ECS/ArchetypePool.hpp"

namespace ECS {

Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo) {
    auto count = signature.count();
    Archetype archetype{
        .signature = signature,
        .componentIndices = {-1},
        .sizes = Alloc<uint16_t>(count),
        .alignments = Alloc<uint16_t>(count),
        .componentIDs = Alloc<ComponentID>(count),
        .numComponents = (int)count,
        .sumSize = 0
    };
    for (int i = 0; i < MaxComponentIDs; i++) {
        archetype.componentIndices[i] = -1;
    }

    int index = 0;
    signature.forEachSet([&](auto componentID){
        auto componentSize = (uint16_t)componentTypeInfo.get(componentID).size;
        archetype.sizes[index] = componentSize;
        archetype.alignments[index] = componentTypeInfo.get(componentID).alignment;
        archetype.sumSize += componentSize;
        archetype.componentIDs[index] = componentID;
        archetype.componentIndices[componentID] = index++;
    });
    return archetype;
}

ArchetypePool::ArchetypePool(const Archetype& archetype)
: archetype(archetype) {
    entities = nullptr;
    buffer = nullptr;
    bufferOffsets = My::Vec<int>::Filled(archetype.numComponents, -1);
    size = 0;
    capacity = 0;
}

int ArchetypePool::addNew(Entity entity) {
    if (size + 1 > capacity) {
        int newCapacity = (capacity * 2 > size) ? capacity * 2 : size + 1;
        // TODO: URGENT: this is horrible
        char* newBuffer = Alloc<char>(newCapacity * archetype.sumSize + 100);
        if (newBuffer) {
            int offset = 0;
            for (int i = 0; i < archetype.numComponents; i++) {
                auto oldComponentBufferSize = archetype.sizes[i] * capacity;
                auto newComponentBufferSize = archetype.sizes[i] * newCapacity;

                size_t alignmentOffset = getAlignmentOffset(offset, archetype.alignments[i]);
                if (buffer)
                    memcpy(newBuffer + offset + alignmentOffset, buffer + bufferOffsets[i], oldComponentBufferSize);

                bufferOffsets[i] = offset + alignmentOffset;
                offset += newComponentBufferSize + alignmentOffset;
            }

            buffer = newBuffer;
        }
        
        Entity* newEntities = Realloc(entities, newCapacity);
        if (newEntities) {
            entities = newEntities;
        }
        capacity = newCapacity;
    }

    // dirty by default
    entities[size] = entity;
    
    return size++;
}

void ArchetypePool::doubleCopyIndex(int dstIndex, int middleIndex, int srcIndex) {
    for (int i = 0; i < archetype.numComponents; i++) {
        auto componentSize = archetype.sizes[i];
        char* componentBuffer = buffer + bufferOffsets[i];

        // copy middle to dst
        memcpy(componentBuffer + dstIndex * componentSize, componentBuffer + middleIndex * componentSize, componentSize);
        // then copy src to middle
        memcpy(componentBuffer + middleIndex * componentSize, componentBuffer + srcIndex * componentSize, componentSize);
    }
    entities[dstIndex] = entities[middleIndex];
    entities[middleIndex] = entities[srcIndex];
}

void ArchetypePool::remove(int index, Entity* movedEntity) {
    assert(index < size);

    const int lastIndex = size - 1;
    if (index < size-1) {
        // much simpler
        *movedEntity = entities[lastIndex];
        copyIndex(index, lastIndex);
    } else {
        // removing very last entity.
        // doesn't matter whether it's dirty or not,
        // no swapping to do
        *movedEntity = NullEntity;
    }

    size--;
}

}