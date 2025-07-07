#ifndef ECS_ARCHETYPE_POOL_INCLUDED
#define ECS_ARCHETYPE_POOL_INCLUDED

#include <array>
#include "llvm/ArrayRef.h"
#include "My/SparseSets.hpp"
#include "My/HashMap.hpp"
#include "My/Bitset.hpp"
#include "utils/ints.hpp"
#include "Entity.hpp"
#include "Signature.hpp"
#include "ComponentInfo.hpp"
#include "utils/Allocator.hpp"

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
    Sint8 componentIndices[MaxComponentID];
    int32_t numComponents;
    Signature signature;
    uint16_t* sizes;
    uint16_t* alignments;
    ComponentID* componentIDs;
    int32_t sumSize; // size of all components added

    static Archetype Null() {
        return {{-1}, 0, 0, 0, nullptr};
    }

    Sint8 getIndex(ComponentID component) const {
        if (component < 0 || component >= MaxComponentID) return -1;
        return componentIndices[component];
    }

    Uint16 getSize(ComponentID component) const {
        return sizes[componentIndices[component]];
    }
};

static Archetype makeArchetype(Signature signature, ComponentInfoRef componentTypeInfo) {
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
    for (int i = 0; i < MaxComponentID; i++) {
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

struct ArchetypePool {
    int size;
    int capacity;
    Entity* entities; // contained entities
    char* buffer;
    My::Vec<int> bufferOffsets; // make llvm::SmallVector?
    Archetype archetype;
    
    ArchetypePool(const Archetype& archetype) : archetype(archetype) {
        entities = nullptr;
        buffer = nullptr;
        bufferOffsets = My::Vec<int>::Filled(archetype.numComponents, -1);
        size = 0;
        capacity = 0;
    }

    int numBuffers() const {
        return bufferOffsets.size;
    }

    char* getBuffer(int bufferIndex) const {
        if (bufferIndex < 0 || bufferIndex >= numBuffers()) {
            LogError("Buffer index out of range!");
            return nullptr;
        }
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
        return getComponentByIndex(bufferIndex, index);
    }

    // returns index where entity is stored
    int addNew(Entity entity) {
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

        entities[size] = entity;
        
        return size++;
    }

    // returns entity that had to be moved to adjust
    Entity remove(int index) {
        assert(index < size);

        // if last entity
        if (index == size-1) {
            size--;
            return NullEntity;
        }
        Entity entityToMove = entities[size-1];
        entities[index] = entityToMove;

        for (int i = 0; i < archetype.numComponents; i++) {
            auto componentSize = archetype.sizes[i];
            memcpy(buffer + bufferOffsets[i] + index * componentSize, buffer + bufferOffsets[i] + (size-1) * componentSize, componentSize);
        }

        size--;
        return entityToMove;
    }

    void destroy() {
        Free(buffer);
        bufferOffsets.destroy();
        Free(entities);
    }
};



struct ArchetypalComponentManager {
    static constexpr size_t MaxEntityID = (1 << 14) - 1;

    using ArchetypeID = Sint16;
    static constexpr ArchetypeID NullArchetypeID = -1;

    My::Vec<ArchetypePool> pools;
    My::Vec<Entity> unusedEntities;
    My::HashMap<Signature, ArchetypeID, SignatureHash> archetypes;
    ComponentInfoRef componentInfo;
    Signature validComponents;

    struct EntityData {
        Sint32 prototype;
        Signature signature;
        Uint32 version;
        Sint16 poolIndex;
        ArchetypeID archetype;
    };

    My::DenseSparseSet<EntityID, EntityData, 
        Uint16, MaxEntityID> entityData;

    ArchetypalComponentManager() {}

    ArchetypalComponentManager(ComponentInfoRef componentInfo) : componentInfo(componentInfo) {
        validComponents = {0};
        for (int i = 0; i < componentInfo.size(); i++) {
            if (!componentInfo.get(i).prototype)
                validComponents.set(i);
        }

        auto nullPool = ArchetypePool(Archetype::Null());
        pools = My::Vec<ArchetypePool>(&nullPool, 1);
        archetypes = decltype(archetypes)::Empty();
        entityData = My::DenseSparseSet<EntityID, EntityData, 
            Uint16, MaxEntityID>::WithCapacity(64);
        unusedEntities = My::Vec<Entity>::WithCapacity(1000);
        unusedEntities.require(1000);
        for (int i = 0; i < 1000; i++) {
            unusedEntities[i].id = 1000 - i - 1;
            unusedEntities[i].version = 0;
        }
    }

    Entity newEntity(Uint32 prototype) {
        Entity entity = unusedEntities.popBack();
        EntityData* data = entityData.insert(entity.id);
        data->version = ++entity.version;
        data->archetype = 0;
        data->poolIndex = -1;
        data->signature = {0};
        data->prototype = prototype;
        return entity;
    }

    void deleteEntity(Entity entity) {
        if (entity.Null()) return;

        const EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
            // entity isn't around anyway
            return;
        }

        ArchetypePool* pool = &pools[data->archetype];
        pool->remove(data->poolIndex);

        entityData.remove(entity.id);
        unusedEntities.push(entity);
    }

    const EntityData* getEntityData(EntityID entityID) const {
        if (entityID == NullEntity.id) {
            return nullptr;
        }
        return entityData.lookup(entityID);
    }

    Signature getEntitySignature(Entity entity) const {
        if (entity.id == NullEntity.id) {
            return {0};
        }
        const EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
            return {0};
        }

        return data->signature;
    }

    bool hasComponent(Entity entity, ComponentID component) const {
       if (entity.id == NullEntity.id) {
            return 0;
        }
        const EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
            return false;
        }

        return data->signature[component];
    }

    void* getComponent(Entity entity, ComponentID component) const {
        if (entity.id == NullEntity.id) {
            return nullptr;
        }
        const EntityData* data = entityData.lookup(entity.id);
        if (!data) {
            return nullptr;
        } else if (entity.version != data->version) {
            LogError("Entity no longer exists!");
            return nullptr;
        }

        if (!data->signature[component]) {
            // entity does not have component
            return nullptr;
        }

        const auto& pool = pools[data->archetype];
        return pool.getComponent(component, data->poolIndex);
    }

    // returns true on success, false on failure
    bool addSignature(Entity entity, Signature components) {
        if (entity.id == NullEntity.id) {
            return false;
        }
        EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
            return false;
        }

        Signature oldSignature = data->signature;
        data->signature |= components;

        auto newArchetypeID = getArchetypeID(data->signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(data->signature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        auto oldArchetypeID = data->archetype;
        if (newArchetypeID == oldArchetypeID) {
            // no new components in signature
            return true;
        }

        int newEntityIndex = newArchetype->addNew(entity);
        
        if (oldArchetypeID > 0) {
            int oldEntityIndex = data->poolIndex;
            ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
            for (int i = 0; i < oldArchetype->numBuffers(); i++) {
                auto componentSize = oldArchetype->archetype.sizes[i];
                ComponentID transferComponent = oldArchetype->archetype.componentIDs[i];
                int newArchetypeIndex = newArchetype->archetype.getIndex(transferComponent);
                assert(newArchetypeIndex != -1);
                char* newComponentAddress = newArchetype->getBuffer(newArchetypeIndex) + componentSize * newEntityIndex;
                char* oldComponentAddress = oldArchetype->getBuffer(i) + componentSize * oldEntityIndex;
                memcpy(newComponentAddress, oldComponentAddress, componentSize);
            }

            Entity movedEntity = oldArchetype->remove(oldEntityIndex);
            if (!movedEntity.Null()) {
                EntityData* movedEntityData = entityData.lookup(movedEntity.id);
                assert(movedEntityData);
                movedEntityData->poolIndex = oldEntityIndex;
            }
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newEntityIndex;

        return true;
    }

    void* addComponent(Entity entity, ComponentID component, const void* initializationValue = nullptr) {
        if (entity.id == NullEntity.id) {
            return nullptr;
        }
        EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
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
            // tried to add component that the entity already has
            // just return the pointer to the component
            return newArchetype->getComponent(component, data->poolIndex);
        }

        int newEntityIndex = newArchetype->addNew(entity);
        
        if (oldArchetypeID > 0) {
            int oldEntityIndex = data->poolIndex;
            ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
            for (int i = 0; i < oldArchetype->numBuffers(); i++) {
                auto componentSize = oldArchetype->archetype.sizes[i];
                ComponentID transferComponent = oldArchetype->archetype.componentIDs[i];
                int newArchetypeIndex = newArchetype->archetype.getIndex(transferComponent);
                assert(newArchetypeIndex != -1);
                char* newComponentAddress = newArchetype->getBuffer(newArchetypeIndex) + componentSize * newEntityIndex;
                char* oldComponentAddress = oldArchetype->getBuffer(i) + componentSize * oldEntityIndex;
                memcpy(newComponentAddress, oldComponentAddress, componentSize);
            }

            Entity movedEntity = oldArchetype->remove(oldEntityIndex);
            if (!movedEntity.Null()) {
                EntityData* movedEntityData = entityData.lookup(movedEntity.id);
                assert(movedEntityData);
                movedEntityData->poolIndex = oldEntityIndex;
            }
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newEntityIndex;

        void* newComponentValue = newArchetype->getComponent(component, newEntityIndex);
        if (initializationValue) {
            assert(newComponentValue);
            memcpy(newComponentValue, initializationValue, componentInfo.get(component).size);
        }

        return newComponentValue;
    }

    void removeComponent(Entity entity, ComponentID component) {
        if (entity.id == NullEntity.id) {
            return;
        }
        EntityData* data = entityData.lookup(entity.id);
        if (!data || (entity.version != data->version)) {
            return;
        }

        Signature oldSignature = data->signature;
        if (!oldSignature[component]) {
            // entity didn't have the component. just do nothing
            return;
        }

        data->signature.set(component, 0);

        auto newArchetypeID = getArchetypeID(data->signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(data->signature);
        }

        ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
        int newEntityIndex = newArchetype->addNew(entity);

        auto oldArchetypeID = data->archetype;
        if (oldArchetypeID > 0) {
            int oldEntityIndex = data->poolIndex;
            ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
            for (int i = 0; i < newArchetype->numBuffers(); i++) {
                auto componentSize = newArchetype->archetype.sizes[i];
                ComponentID transferComponent = newArchetype->archetype.componentIDs[i];
                int oldArchetypeIndex = oldArchetype->archetype.getIndex(transferComponent);
                assert(oldArchetypeIndex != -1);
                char* oldComponentAddress = oldArchetype->getBuffer(oldArchetypeIndex) + componentSize * oldEntityIndex;
                char* newComponentAddress = newArchetype->getBuffer(i) + componentSize * newEntityIndex;
                memcpy(newComponentAddress, oldComponentAddress, componentSize);
            }

            Entity movedEntity = oldArchetype->remove(oldEntityIndex);
            if (!movedEntity.Null()) {
                EntityData* movedEntityData = entityData.lookup(movedEntity.id);
                assert(movedEntityData);
                movedEntityData->poolIndex = oldEntityIndex;
            }
        }

        data->archetype = newArchetypeID;
        data->poolIndex = newEntityIndex;
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

    const ComponentInfo& getComponentInfo(ComponentID component) const {
        return componentInfo.get(component);
    }

    void destroy() {
        pools.destroy();
        unusedEntities.destroy();
        archetypes.destroy();
        entityData.destroy();
    }
};

} // namespace ECS

#endif