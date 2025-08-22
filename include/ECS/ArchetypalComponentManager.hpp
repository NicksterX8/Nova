#pragma once

#include "ArchetypePool.hpp"
#include "My/HashMap.hpp"
#include "My/Vec.hpp"

namespace ECS {

 struct ComponentGroup {
    Signature required;
    Signature rejected;

    bool contains(Signature components) const {
        return components.hasAll(required) && components.hasNone(rejected);
        // alternate: doesn't have short circuiting but no branch is probably good?
       // return ((components & required) ^ (components & ~rejected)) == components;
    }

    bool operator==(ComponentGroup other) const {
        return required == other.required && rejected == other.rejected;
    }

    struct Hash {
        My::Map::Hash operator()(ComponentGroup group) const {
            return group.required.words[0] ^ group.rejected.words[0];
        }
    };
};

using GroupWatcherType = uint8_t;

namespace GroupWatcherTypes {
    enum : GroupWatcherType {
        EnteredGroup = 1,
        ExitedGroup = 2,
    };
}

struct EntityCreationError {
    enum Type {
        None,
        InvalidEntity,
        EntityLimitReached
    } type;

    EntityCreationError() : type(None) {}

    EntityCreationError(Type type) : type(type) {}

    constexpr static const char* errorNames[3] = {
        "None",
        "Invalid entity",
        "Entity limit reached"
    };

    const char* name() const {
        return errorNames[type];
    }

    operator bool() const {
        return type != None;
    }
};

// Helper for making strong types.
template <typename T> 
class StrongType {
    T val;
public:
    // template <typename... Args>
    // explicit constexpr StrongType(Args... A) : val(std::forward<Args>(A)...) {}

    StrongType() = default;

    explicit constexpr StrongType(T val) : val(val) {}

    explicit operator T() const {
        return val;
    }

    explicit operator bool() const {
        return (bool)val;
    }

    T operator+(T rhs) const {
        return val + rhs;
    }

    bool operator==(T rhs) const {
        return val == rhs;
    }

    bool operator==(StrongType<T> rhs) const {
        return val == rhs.val;
    }
};

struct MultiEntity {
    Entity entity;
    Sint32 count;
};

constexpr MultiEntity NullMultiEntity = {NullEntity, 0};

using EntityIndex = StrongType<Uint16>;

static constexpr EntityID MaxEntityID = (1 << 15) - 1;

struct ArchetypalComponentManager {
    using ArchetypeID = Sint16;
    static constexpr ArchetypeID NullArchetypeID = -1;

    PoolAllocator poolAllocator;
    ArchetypeAllocator archetypeAllocator;

    SmallVectorA<ArchetypePool, PoolAllocator, 0> pools;
    My::Vec<Entity> unusedEntities;
    EntityID highestUsedEntity = 0;
    My::HashMap<Signature, ArchetypeID, SignatureHash> archetypes;

    Sint32* componentSizes = nullptr;
    const char** componentNames = nullptr;
    int numComponentTypes = 0;

    Signature validComponents = {0};

    struct EntityLoc {
        ArchetypeID archetype;
        Sint16 index;
    };
    static constexpr EntityLoc NullEntityLoc = {0, 0};

    struct EntityData {
        Sint32* prototype = nullptr;
        Uint32* version = nullptr;
        EntityLoc* location = nullptr;
        EntityID* id = nullptr;
    } entityData;

    int entityCount = 0;
    int entityCapacity = 0;

    EntityIndex* entityIndices = nullptr;
    static constexpr EntityIndex NullEntityIndex = EntityIndex(0);
    static const int EntityIndicesLength = MaxEntityID;

    struct ComponentWatcher {
        ComponentGroup group;
        ArchetypePool* pool;
        GroupWatcherType type;
    };

    Signature watchedComponentAdds = 0;
    Signature watchedComponentRemoves = 0;
    SmallVector<SmallVector<ComponentWatcher>, 0> componentAddWatchers;
    SmallVector<SmallVector<ComponentWatcher>, 0> componentRemoveWatchers;
    static constexpr Uint8 NullWatcherIndex = 255;
    Uint8 watchedComponentAddGroupIndices[MaxComponentIDs]; 
    Uint8 watchedComponentRemoveGroupIndices[MaxComponentIDs];

    /* Methods */

    EntityIndex getEntityIndex(EntityID id) const {
        assert(id < EntityIndicesLength);
        return entityIndices[id];
    }

    EntityIndex lookupEntity(Entity entity) const {
        assert(entity.id < EntityIndicesLength);
        EntityIndex index = entityIndices[entity.id];
        auto version = entityData.version[(Uint16)index];
        if (UNLIKELY(version != entity.version)) {
            assert(entity.id == NullEntity.id && "Use of destroyed entity!");
            return NullEntityIndex;
        }
        return index;
    }

    ArchetypePool* getPool(EntityIndex index) {
        auto archetype = entityData.location[(Uint16)index].archetype;
        return &pools[archetype];
    }

    ArchetypePool* getOrMakePool(Signature signature, ArchetypeID* newArchetypeIDOut) {
        auto newArchetypeID = getArchetypeID(signature);
        if (newArchetypeID == NullArchetypeID) {
            newArchetypeID = initArchetype(signature);
        }
        if (newArchetypeIDOut)
            *newArchetypeIDOut = newArchetypeID;
        return getPool(newArchetypeID);
    }

    __attribute__((pure)) Signature getSignature(EntityIndex index) const {
        auto archetype = entityData.location[(Uint16)index].archetype;
        const auto* pool = getPool(archetype);
        return pool->signature();
    }

    EntityVersion getVersion(EntityIndex index) const {
        return entityData.version[(Uint16)index];
    }

    Sint32 getPrototype(EntityIndex index) const {
        return entityData.prototype[(Uint16)index];
    }

    ArchetypeID getArchetype(EntityIndex index) const {
        return entityData.location[(Uint16)index].archetype;
    }

    ArchetypePool::EntityIndex getPoolIndex(EntityIndex index) const {
        return entityData.location[(Uint16)index].index;
    }

    void setEntityLocation(EntityIndex index, EntityLoc entityLocation) {
        entityData.location[(Uint16)index] = entityLocation;
    }

    int addToPool(ArchetypePool* pool, ArrayRef<Entity> entities) {
        return pool->addNew(entities.size(), entities.data(), &archetypeAllocator, componentSizes);
    }

    bool reserveEntities(Sint32 count);

    ArchetypePool* addWatcher(ComponentGroup group, GroupWatcherType type);

    void init(ArrayRef<ComponentInfo> componentInfo, ArenaAllocator* arena);

    Entity createEntity(Uint32 prototype);

    void createEntities(int count, Entity* entitiesOut, Signature signature);

    MultiEntity createMultiEntity(Sint32 count);

    // true on success, false on error
    // will return EntityCreationError::InvalidEntity if entity is null or not in existence
    EntityCreationError clone(Entity entity, int count, Entity* clonesOut);

    void deleteEntity(Entity entity);
    void deleteEntities(ArrayRef<Entity> entities);

    __attribute__((pure)) Signature getEntitySignature(Entity entity) const {
        auto entityIndex = lookupEntity(entity);
        auto* pool = &pools[getArchetype(entityIndex)];
        return pool->signature();
    }

    __attribute__((pure)) bool hasComponent(Entity entity, ComponentID component) const {
        return getEntitySignature(entity)[component];
    }

    __attribute__((pure)) void* getComponent(Entity entity, ComponentID component) const;

    // @return new pool index of entity
    int moveEntityToSuperArchetype(Entity entity, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, int oldPoolIndex);
    int moveEntitiesToSuperArchetype(ArrayRef<Entity> entities, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, const int* oldPoolIndices);
    // @return new pool index of entity
    int moveEntityToSubArchetype(Entity entity, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, int oldPoolIndex);

    void removeEntityIndexFromPool(int index, ArchetypePool* pool);
    void removeEntityIndicesFromPool(ArrayRef<int> indices, ArchetypePool* pool);

protected:
    void signatureAdded(Entity, Signature added, Signature oldSignature);
    void signatureRemoved(Entity, Signature removed, Signature oldSignature);
public:

    // returns true on success, false on failure
    EntityLoc addSignature(Entity entity, Signature components);

    void* addComponent(Entity entity, ComponentID component);

    struct GroupedEntities {
        ArrayRef<Entity> entities;

    };

    void* addComponent(ArrayRef<Entity> groupedEntities, ComponentID component);

    void removeComponent(Entity entity, ComponentID component);

private:
    // returns -1 if the archetype doesn't exist
    ArchetypeID getArchetypeID(Signature signature) const {
        auto* archetypeID = archetypes.lookup(signature);
        if (archetypeID) {
            return *archetypeID;
        }
        return NullArchetypeID;
    }

    const ArchetypePool* getPool(ArchetypeID id) const {
        return &pools[id];
    }

    ArchetypePool* getPool(ArchetypeID id) {
        return &pools[id];
    }
public:

    ArchetypeID initArchetype(Signature signature);

    __attribute__((pure)) Sint32 getComponentSize(ComponentID component) const {
        assert(component < numComponentTypes && "Invalid component!");
        return componentSizes[component];
    }

    void debugCheckComponent(ComponentID component) const;

    void debugCheckSignature(Signature signature) const;

    void destroy();
};

}