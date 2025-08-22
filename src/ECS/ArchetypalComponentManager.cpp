#include "ECS/ArchetypalComponentManager.hpp"

namespace ECS {

void ArchetypalComponentManager::init(ArrayRef<ComponentInfo> componentInfo, ArenaAllocator* arena) {
    numComponentTypes = componentInfo.size();
    componentSizes = ALLOC(Sint32, numComponentTypes, *arena);
    componentNames = ALLOC(const char*, numComponentTypes, *arena);
    for (int i = 0; i < componentInfo.size(); i++) {
        if (componentInfo[i].empty)
            componentSizes[i] = 0;
        else
            componentSizes[i] = componentInfo[i].size;
        componentNames[i] = componentInfo[i].name;
    }

    // make a pool for empty entities to have so they can still be used to check signature and information about entities,
    // although entities are not actually added to this pool (as it would be wasteful)
    auto nullPool = ArchetypePool(Signature{0}, componentSizes, &poolAllocator);
    pools.push_back(nullPool);
    archetypes = decltype(archetypes)::Empty();
    unusedEntities = My::Vec<Entity>::WithCapacity(512);

    memset(watchedComponentAddGroupIndices, NullWatcherIndex, sizeof(watchedComponentAddGroupIndices));
    memset(watchedComponentRemoveGroupIndices, NullWatcherIndex, sizeof(watchedComponentRemoveGroupIndices));

    // calloc setting all to 0 means by default all entity data references nullentity,
    // meaning any uninitialized entity automatically behaves as null without any extra effort
    entityIndices = (EntityIndex*)calloc(MaxEntityID, sizeof(EntityIndex));

    reserveEntities(64);

    // add null entity to entityData
    static_assert(NULL_ENTITY_ID == 0, "wrong null entity index");
    entityData.location[0] = {0, 0};
    entityData.prototype[0] = -1;
    entityData.version[0] = (-1); // so that entity.version == entityData.version is always false for null entity
    entityData.id[0] = NullEntity.id;
    entityCount++;
    highestUsedEntity++;
}

bool ArchetypalComponentManager::reserveEntities(Sint32 count) {
    if (UNLIKELY(entityCount + count > MaxEntityID)) {
        LogError("Failed to reserve entities!");
        return false;
    }
    if (entityCapacity < entityCount + count) {
        int newCapacity = MAX(entityCapacity * 2, entityCount + count);
        entityData.prototype = Realloc<Sint32>(entityData.prototype, newCapacity);
        entityData.location = Realloc<EntityLoc>(entityData.location, newCapacity);
        entityData.version = Realloc<EntityVersion>(entityData.version, newCapacity);
        entityData.id = Realloc<EntityID>(entityData.id, newCapacity);
        entityCapacity = newCapacity;
    }
    return true;
}

void ArchetypalComponentManager::debugCheckComponent(ComponentID component) const {
    assert(component < this->numComponentTypes && component >= 0 && "Component not known by component manager!");
}

void ArchetypalComponentManager::debugCheckSignature(Signature signature) const {
    assert(signature.highestSet() < this->numComponentTypes && "Component not known by component manager!");
}

ArchetypePool* ArchetypalComponentManager::addWatcher(ComponentGroup group, GroupWatcherType type) {
    // having more than one of the same group is just inefficient and pointless
    // actually removing for now because it's not really pointless, since 
    // there could be issues if we share pools between systems/groups.
    // we can just let the caller of addWatcher try to optimize this if they want
    // if (watcherPools.contains(group)) return;

    if (type & GroupWatcherTypes::EnteredGroup) {
        watchedComponentAdds |= group.required;
        watchedComponentRemoves |= group.rejected;
    }
    if (type & GroupWatcherTypes::ExitedGroup) {
        // we need to track when a component required in the group is removed
        // so we know when it exits the group
        watchedComponentRemoves |= group.required;
        // we need to know when a component is added so we can track if it was rejected
        watchedComponentAdds |= group.rejected;
    }

    auto* pool = NEW(ArchetypePool({0}, componentSizes, &poolAllocator), poolAllocator);
    ComponentWatcher watcher = {
        group,
        pool,
        type
    };

    if (type & GroupWatcherTypes::EnteredGroup) {
        group.required.forEachSet([this, watcher](ComponentID component){
            Uint8& componentWatcherListIndex = watchedComponentAddGroupIndices[component];
            if (componentWatcherListIndex == NullWatcherIndex) {
                componentWatcherListIndex = componentAddWatchers.size();
                componentAddWatchers.push_back({});
            }
            componentAddWatchers[componentWatcherListIndex].push_back(watcher);
        });
        group.rejected.forEachSet([this, watcher](ComponentID component){
            Uint8& componentWatcherListIndex = watchedComponentRemoveGroupIndices[component];
            if (componentWatcherListIndex == NullWatcherIndex) {
                componentWatcherListIndex = componentRemoveWatchers.size();
                componentRemoveWatchers.push_back({});
            }
            componentRemoveWatchers[componentWatcherListIndex].push_back(watcher);
        });
    }
    if (type & GroupWatcherTypes::ExitedGroup) {
        group.required.forEachSet([this, watcher](ComponentID component){
            Uint8& componentWatcherListIndex = watchedComponentRemoveGroupIndices[component];
            if (componentWatcherListIndex == NullWatcherIndex) {
                componentWatcherListIndex = componentRemoveWatchers.size();
                componentRemoveWatchers.push_back({});
            }
            componentRemoveWatchers[componentWatcherListIndex].push_back(watcher);
        });
        group.rejected.forEachSet([this, watcher](ComponentID component){
            Uint8& componentWatcherListIndex = watchedComponentAddGroupIndices[component];
            if (componentWatcherListIndex == NullWatcherIndex) {
                componentWatcherListIndex = componentAddWatchers.size();
                componentAddWatchers.push_back({});
            }
            componentAddWatchers[componentWatcherListIndex].push_back(watcher);
        });
    }

    return pool;
}

// Entity ArchetypalComponentManager::getUnusedEntity() {
    
// }

Entity ArchetypalComponentManager::createEntity(Uint32 prototype) {
    Entity entity;
    if (!unusedEntities.empty()) {
        entity = unusedEntities.popBack();
    } else if (highestUsedEntity < MaxEntityID) {
        entity.id = highestUsedEntity++;
        entity.version = 1;
    } else {
        LogCrash(CrashReason::UnrecoverableError, "All possible entity IDs used!");
    }

    reserveEntities(1);

    auto entityIndex = EntityIndex(entityCount++);
    entityIndices[entity.id] = entityIndex;
    entityData.version[(Uint16)entityIndex] = entity.version;
    entityData.prototype[(Uint16)entityIndex] = prototype;
    entityData.location[(Uint16)entityIndex] = {
        .archetype = 0,
        .index = 0
    };
    entityData.id[(Uint16)entityIndex] = entity.id;

    return entity;
}

MultiEntity ArchetypalComponentManager::createMultiEntity(Sint32 count) {
    if (count < 1) return NullMultiEntity;
    return {createEntity(-1), count};
}

EntityCreationError ArchetypalComponentManager::clone(Entity entity, int count, Entity* clonesOut) {
    if (count <= 0) return {};
    assert(clonesOut && "null pointer provided for clone output");

    EntityIndex entityIndex = lookupEntity(entity);
    if (!entityIndex) return {EntityCreationError::InvalidEntity};

    auto location = entityData.location[(Uint16)entityIndex];
    auto archetype = location.archetype;
    auto* pool = &pools[archetype];

    Signature entitySignature = pool->signature();

    reserveEntities(count);

    EntityID unusedIDs = (MaxEntityID - 1) - highestUsedEntity;
    EntityIndex clonesFirstIndex;
    Entity* clones = nullptr;
    if (unusedIDs >= count) {
        EntityID firstID = highestUsedEntity + 1;
        clonesFirstIndex = EntityIndex(entityCount);
        for (int i = 0; i < count; i++) {
            entityIndices[firstID + i] = EntityIndex(clonesFirstIndex + i);
            clonesOut[i] = {firstID + i, 1};
        }
        clones = clonesOut;

        highestUsedEntity = firstID + count - 1;
    } else if (unusedEntities.size >= count) {
        clones = unusedEntities.end() - count;
        memcpy(clonesOut, clones, count * sizeof(Entity));
        unusedEntities.size -= count;

        Sint32 clonesFirstIndex = entityCount;
        for (int i = 0; i < count; i++) {
            entityIndices[clones[i].id] = EntityIndex(clonesFirstIndex + i);
        }

    } else {
        return {EntityCreationError::EntityLimitReached};
    }

    for (int i = 0; i < count; i++) {
        entityData.location[clonesFirstIndex  + i] = entityData.location[(Uint16)entityIndex];
        entityData.prototype[clonesFirstIndex + i] = entityData.prototype[(Uint16)entityIndex];
        entityData.version[clonesFirstIndex   + i] = clones[i].version;
    }

    if (!pool->null()) {
        int startPoolIndex = pool->addNew(count, clones, &archetypeAllocator, componentSizes);
        for (int i = 0; i < count; i++) {
            entityData.location[clonesFirstIndex+i].index = startPoolIndex + i;

            signatureAdded(clones[i], entitySignature, {0});
        }

        entitySignature.forEachSet([this, count, pool, entityIndex](ComponentID componentID){
            auto size = this->componentSizes[componentID];
            void* component = pool->getComponent(componentID, entityData.location[(Uint16)entityIndex].index, size);
            void* clonesStart = pool->getComponent(componentID, pool->size - count, size);
            for (int i = 0; i < count; i++) {
                void* cloneComponent = (char*)clonesStart + i * size;
                memcpy(cloneComponent, component, size);
            }
        });
    } else {
        // maybe unnecessary, since we shouldn't be accessing this anyway if there isn't an archetype/pool for the entity?
        for (int i = 0; i < count; i++) {
            entityData.location[clonesFirstIndex + i].index = 0;
        }
    }

    return {};
}

#define GOOD_ENTITY(entity, index) auto trueVersion = getVersion(index); (LIKELY(entity.version == getVersion(index)))

void* ArchetypalComponentManager::getComponent(Entity entity, ComponentID component) const {
    auto index = lookupEntity(entity);

    const auto& pool = pools[getArchetype(index)];
    auto poolIndex = getPoolIndex(index);
    char* array = pool.getComponentArray(component);
    if (!array) return nullptr; // doesnt have it
    return array + poolIndex * getComponentSize(component);
}

void ArchetypalComponentManager::removeEntityIndexFromPool(int index, ArchetypePool* pool) {
    assert(pool);
    Entity movedEntity;
    pool->remove(index, &movedEntity, componentSizes);
    if (movedEntity.NotNull()) {
        auto movedEntityIndex = entityIndices[movedEntity.id];
        entityData.location[(Uint16)movedEntityIndex].index = index;
    }
}

void ArchetypalComponentManager::removeEntityIndicesFromPool(ArrayRef<int> indices, ArchetypePool* pool) {
    for (auto index : indices) {
        removeEntityIndexFromPool(index, pool);
    }
}

int ArchetypalComponentManager::moveEntityToSuperArchetype(Entity entity, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, int oldPoolIndex) {
    assert(oldArchetype && newArchetype);
    int newPoolIndex = addToPool(newArchetype, entity);

    if (oldArchetype->null()) {
        // nothing to move
        return newPoolIndex;
    }

    for (int i = 0; i < oldArchetype->numComponentArrays(); i++) {
        ComponentID transferComponent = oldArchetype->arrays[i].componentType;
        auto componentSize = componentSizes[transferComponent];
        const void* oldComponent = oldArchetype->getComponentByIndex(i, oldPoolIndex, componentSize);
        void* newComponent = newArchetype->getComponent(transferComponent, newPoolIndex, componentSize);
        assert(oldComponent && newComponent);
        memcpy(newComponent, oldComponent, componentSize);
    };

    removeEntityIndexFromPool(oldPoolIndex, oldArchetype);
    return newPoolIndex;
}


int ArchetypalComponentManager::moveEntitiesToSuperArchetype(ArrayRef<Entity> entities, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, const int* oldPoolIndices) {
    assert(oldArchetype && newArchetype);
    int newPoolStartIndex = addToPool(newArchetype, entities);

    if (oldArchetype->null()) {
        // no components to move, we are done
        return newPoolStartIndex;
    }

    for (int i = 0; i < oldArchetype->numComponentArrays(); i++) {
        ComponentID transferComponent = oldArchetype->arrays[i].componentType;
        auto componentSize = componentSizes[transferComponent];
        char* newComponents = newArchetype->getComponent(transferComponent, newPoolStartIndex, componentSize);
        assert(newComponents);
        for (int i = 0; i < entities.size(); i++) {
            const char* oldComponent = oldArchetype->getComponentByIndex(i, oldPoolIndices[i], componentSize);
            memcpy(newComponents + i * componentSize, oldComponent, componentSize);
        }
    };

    removeEntityIndicesFromPool({oldPoolIndices, entities.size()}, oldArchetype);
    return newPoolStartIndex;
}

int ArchetypalComponentManager::moveEntityToSubArchetype(Entity entity, ArchetypePool* oldArchetype, ArchetypePool* newArchetype, int oldPoolIndex) {
    if (newArchetype->null()) {
        // nothing to move
        return 0;
    }
    
    int newPoolIndex = addToPool(newArchetype, entity);
    for (int i = 0; i < newArchetype->numComponentArrays(); i++) {
        ComponentID transferComponent = newArchetype->arrays[i].componentType;
        auto componentSize = componentSizes[transferComponent];
        void* newComponent = newArchetype->getComponentByIndex(i, newPoolIndex, componentSize);
        const void* oldComponent = oldArchetype->getComponent(transferComponent, oldPoolIndex, componentSize);
        assert(oldComponent && newComponent);
        memcpy(newComponent, oldComponent, componentSize);
    };

    removeEntityIndexFromPool(oldPoolIndex, oldArchetype);
    return newPoolIndex;
}

void* ArchetypalComponentManager::addComponent(Entity entity, ComponentID component) {
    debugCheckComponent(component);

    assert(entity.id != NullEntity.id && "Tried to add component to null entity!");
    auto entityIndex = lookupEntity(entity);
    if (entityIndex == 0) return nullptr;

    ArchetypeID oldArchetypeID = getArchetype(entityIndex);
    auto oldPoolIndex = getPoolIndex(entityIndex);
    Signature oldSignature = getPool(oldArchetypeID)->signature();
    if (oldSignature[component]) {
        // already have this component
        // act as get instead
        return getPool(oldArchetypeID)->getComponent(component, oldPoolIndex, getComponentSize(component));
    }

    Signature newSignature = oldSignature; newSignature.set(component);

    ArchetypeID newArchetypeID;
    // all archetype pool pointers are invalidated by this call
    ArchetypePool* newArchetype = getOrMakePool(newSignature, &newArchetypeID);

    // don't waste time for components that have no groups associated with them
    signatureAdded(entity, Signature::OneComponent(component), oldSignature);

    int newPoolIndex = moveEntityToSuperArchetype(entity, getPool(oldArchetypeID), newArchetype, oldPoolIndex);

    setEntityLocation(entityIndex, {newArchetypeID, (Sint16)newPoolIndex}); // new archetype pushed back last

    return newArchetype->getComponent(component, newPoolIndex, componentSizes[component]);
}

void ArchetypalComponentManager::signatureAdded(Entity entity, Signature addedSignature, Signature oldSignature) {
    Signature newSignature = oldSignature | addedSignature;
    Signature watchedAdds = addedSignature & watchedComponentAdds;
    if (watchedAdds.any()) {
        for (auto& watcher : componentAddWatchers[0]) {
            bool inGroupNow = watcher.group.contains(newSignature);
            bool wasInGroup = watcher.group.contains(oldSignature);
            if (inGroupNow && !wasInGroup) {
                if (watcher.type & GroupWatcherTypes::EnteredGroup) {
                    addToPool(watcher.pool, entity);
                }
            } else if (!inGroupNow && wasInGroup) {
                if (watcher.type & GroupWatcherTypes::ExitedGroup) {
                    addToPool(watcher.pool, entity);
                }
            }
        }
    }
}

void ArchetypalComponentManager::signatureRemoved(Entity entity, Signature removedSignature, Signature oldSignature) {
    Signature newSignature = oldSignature ^ removedSignature;
    Signature watchedRemoves = removedSignature & watchedComponentRemoves;
    if (watchedRemoves.any()) {
        for (auto& watcher : componentAddWatchers[0]) {
            bool inGroupNow = watcher.group.contains(newSignature);
            bool wasInGroup = watcher.group.contains(oldSignature);
            if (inGroupNow && !wasInGroup) {
                if (watcher.type & GroupWatcherTypes::EnteredGroup) {
                    addToPool(watcher.pool, entity);
                }
            } else if (!inGroupNow && wasInGroup) {
                if (watcher.type & GroupWatcherTypes::ExitedGroup) {
                    addToPool(watcher.pool, entity);
                }
            }
        }
    }
}

ArchetypalComponentManager::EntityLoc ArchetypalComponentManager::addSignature(Entity entity, Signature components) {
    debugCheckSignature(components);

    assert(entity.id != NullEntity.id && "Tried to add component to null entity!");
    auto entityIndex = lookupEntity(entity);
    if (entityIndex == NullEntityIndex) return NullEntityLoc;

    auto oldArchetypeID = getArchetype(entityIndex);
    auto oldPoolIndex = getPoolIndex(entityIndex);
    Signature oldSignature = getPool(oldArchetypeID)->signature();
    if (oldSignature.hasAll(components)) {
        // already have all these components
        // act as get instead
        return {oldArchetypeID, oldPoolIndex};
    }

    Signature newSignature = oldSignature | components;
    ArchetypeID newArchetypeID;
    ArchetypePool* newArchetype = getOrMakePool(newSignature, &newArchetypeID);

    // don't waste time for components that have no groups associated with them
    signatureAdded(entity, components, oldSignature);

    int newPoolIndex = moveEntityToSuperArchetype(entity, getPool(oldArchetypeID), newArchetype, oldPoolIndex);

    auto location = EntityLoc{newArchetypeID, (Sint16)newPoolIndex};
    setEntityLocation(entityIndex, location);

    return location;
}

void ArchetypalComponentManager::removeComponent(Entity entity, ComponentID component) {
    assert(entity.id != NullEntity.id && "Can't remove component from null entity!");
    auto entityIndex = lookupEntity(entity);

    auto oldArchetypeID = getArchetype(entityIndex);
    auto oldPoolIndex = getPoolIndex(entityIndex);

    Signature oldSignature = getPool(oldArchetypeID)->signature();
    DASSERT(oldSignature[component]);
    Signature newSignature = oldSignature; newSignature.flipBit(component);
    ArchetypeID newArchetypeID;
    ArchetypePool* newArchetype = getOrMakePool(newSignature, &newArchetypeID);
    
    signatureRemoved(entity, Signature::OneComponent(component), oldSignature);

    int newPoolIndex = moveEntityToSubArchetype(entity, getPool(oldArchetypeID), newArchetype, oldPoolIndex);
    auto location = EntityLoc{newArchetypeID, (Sint16)newPoolIndex};
    setEntityLocation(entityIndex, location);
} 

void ArchetypalComponentManager::deleteEntity(Entity entity) {
    auto entityIndex = lookupEntity(entity);
    if (!entityIndex) return;

    ArchetypePool* pool = &pools[getArchetype(entityIndex)];
    if (pool && !pool->null()) {
        auto poolIndex = getPoolIndex(entityIndex);
        auto signature = getSignature(entityIndex);

        signatureRemoved(entity, signature, signature);

        removeEntityIndexFromPool(poolIndex, pool);
    }

    // reuse the entity index
    // move the top most entity to this entities position
    auto topEntity = EntityIndex(entityCount-1);
    entityData.location[(Uint16)entityIndex] = entityData.location[(Uint16)topEntity];
    entityData.version[(Uint16)entityIndex] = entityData.version[(Uint16)topEntity];
    entityData.prototype[(Uint16)entityIndex] = entityData.prototype[(Uint16)topEntity];
    auto topEntityID = entityData.id[(Uint16)topEntity];
    entityIndices[topEntityID] = entityIndex;
    entityIndices[entity.id] = NullEntityIndex;
    unusedEntities.push({entity.id, entity.version + 1});
    entityCount--;
}

void ArchetypalComponentManager::deleteEntities(ArrayRef<Entity> entities) {
    // TODO: OPTIMIZE: optimize for many entities
    for (int i = 0; i < entities.size(); i++) {
        deleteEntity(entities[i]);
    }
}

ArchetypalComponentManager::ArchetypeID ArchetypalComponentManager::initArchetype(Signature signature) {
    DASSERT(!archetypes.contains(signature));

    pools.emplace_back(signature, componentSizes, &poolAllocator);
    archetypes.insert(signature, pools.size()-1);
    return pools.size()-1;
}

void ArchetypalComponentManager::destroy() {
    unusedEntities.destroy();
    for (auto& pool : pools) {
        pool.destroy(&poolAllocator, &archetypeAllocator, componentSizes);
    }
    archetypes.destroy();
    free(entityIndices);
}

}