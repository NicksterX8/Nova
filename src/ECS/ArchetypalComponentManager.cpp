#include "ECS/ArchetypalComponentManager.hpp"

namespace ECS {

void ArchetypalComponentManager::init(ComponentInfoRef componentInfo) {
    this->componentInfo = componentInfo;
    for (int i = 0; i < componentInfo.size(); i++) {
        if (!componentInfo.get(i).prototype)
            validComponents.set(i);
    }

    auto nullPool = ArchetypePool(Archetype::Null());
    pools.push_back(nullPool);
    archetypes = decltype(archetypes)::Empty();
    entityData = My::DenseSparseSet<EntityID, EntityData, 
        Uint16, MaxEntityID>::WithCapacity(64);
    unusedEntities = My::Vec<Entity>::WithCapacity(512);

    memset(watchedComponentAddGroupIndices, NullWatcherIndex, sizeof(watchedComponentAddGroupIndices));
    memset(watchedComponentRemoveGroupIndices, NullWatcherIndex, sizeof(watchedComponentRemoveGroupIndices));
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

    auto* pool = NEW(ArchetypePool(makeArchetype(0, componentInfo)));
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

Entity ArchetypalComponentManager::createEntity(Uint32 prototype) {
    Entity entity;
    if (!unusedEntities.empty()) {
        entity = unusedEntities.popBack();
    } else if (highestUsedEntity < MaxEntityID) {
        entity.id = ++highestUsedEntity;
        entity.version = 0;
    } else {
        LogCritical("All possible entity IDs used!!");
        abort();
    }
    EntityData* data = entityData.insert(entity.id);
    data->version = ++entity.version;
    data->archetype = 0;
    data->poolIndex = -1;
    data->signature = {0};
    data->prototype = prototype;
    return entity;
}

void* ArchetypalComponentManager::getComponent(Entity entity, ComponentID component) const {
    if (entity.id == NullEntity.id) {
        return nullptr;
    }
    const EntityData* data = entityData.lookup(entity.id);
    if (!data) {
        // the entity was deleted and isn't being reused currently
        LogError("Entity no longer exists!");
        return nullptr;
    } else if (entity.version != data->version) {
        // the entity was deleted and is being used currently
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

void ArchetypalComponentManager::removeEntityIndexFromPool(int index, ArchetypePool* pool) {
    Entity movedEntity;
    pool->remove(index, &movedEntity);
    if (movedEntity.NotNull()) {
        EntityData* data = entityData.lookup(movedEntity.id);
        data->poolIndex = index;
    }
}

void ArchetypalComponentManager::addComponent(Entity entity, ComponentID component, const void* initializationValue) {
    assert(entity.id != NullEntity.id && "Tried to add component to null entity!");
    EntityData* data = entityData.lookup(entity.id);

    assert(data && "Entity not in entity map!");
    assert(entity.version == data->version && "Entity with incorrect version!");

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
        // just set the value
        void* componentPtr = getComponent(entity, component);
        memcpy(componentPtr, initializationValue, componentInfo.get(component).size);
        return;
    }

    // don't waste time for components that have no groups associated with them
    if (watchedComponentAdds[component]) {
        Signature signature = newArchetype->signature();
        Uint8 addGroupsIndex = watchedComponentAddGroupIndices[component];
        auto& groupList = componentAddWatchers[addGroupsIndex];
        for (auto& watcher : groupList) {
            if (watcher.type & GroupWatcherTypes::EnteredGroup) {
                if (watcher.group.contains(signature)) {
                    watcher.pool->addNew(entity);
                    continue; // can't enter and exit a group at the same time
                }
            }
            if (watcher.type & GroupWatcherTypes::ExitedGroup) {
                if (watcher.group.contains(oldSignature)) {
                    watcher.pool->addNew(entity);
                }
            }
        }
    }

    int poolIndex = newArchetype->addNew(entity);

    if (oldArchetypeID > 0) {
        int oldEntityIndex = data->poolIndex;
        ArchetypePool* oldArchetype = getArchetypePool(oldArchetypeID);
        Signature sharedComponents = newArchetype->signature() & oldArchetype->signature();
        for (int i = 0; i < oldArchetype->numBuffers(); i++) {
            ComponentID transferComponent = oldArchetype->archetype.componentIDs[i];
            auto componentSize = getComponentInfo(transferComponent).size;

            int newComponentIndex = newArchetype->archetype.getIndex(transferComponent);
            int oldComponentIndex = oldArchetype->archetype.getIndex(transferComponent);
            assert(newComponentIndex != -1);
            assert(oldComponentIndex != -1);

            char* oldComponentAddress = oldArchetype->getBuffer(oldComponentIndex) + componentSize * oldEntityIndex;
            char* newComponentAddress = newArchetype->getBuffer(newComponentIndex) + componentSize * poolIndex;
            memcpy(newComponentAddress, oldComponentAddress, componentSize);
        };

        removeEntityIndexFromPool(oldEntityIndex, oldArchetype);
    }

    if (initializationValue) {
        auto componentSize = getComponentInfo(component).size;
        void* newComponentValue = newArchetype->getComponent(component, poolIndex);
        assert(newComponentValue && "We just added, it should exist!");
        memcpy(newComponentValue, initializationValue, componentSize);
    }

    data->archetype = newArchetypeID;
    data->poolIndex = poolIndex; // new archetype pushed back last
}

bool ArchetypalComponentManager::addSignature(Entity entity, Signature components) {
    if (entity.id == NullEntity.id) {
        return false;
    }
    EntityData* data = entityData.lookup(entity.id);
    if (!data || (entity.version != data->version)) {
        return false;
    }

    Signature oldSignature = data->signature;
    Signature newSignature = oldSignature | components;

    auto newArchetypeID = getArchetypeID(newSignature);
    if (newArchetypeID == NullArchetypeID) {
        newArchetypeID = initArchetype(newSignature);
    }

    ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
    auto oldArchetypeID = data->archetype;
    if (newArchetypeID == oldArchetypeID) {
        // no new components in signature
        return true;
    }

    int newEntityIndex = newArchetype->addNew(entity);

    Signature watchedAdds = components & watchedComponentAdds;
    if (watchedAdds.any()) {
        SmallVector<ArchetypePool*> triggeredPools;
        watchedAdds.forEachSet([this, entity, oldSignature, newSignature, &triggeredPools](ComponentID component){
            auto index = watchedComponentAddGroupIndices[component];
            for (auto& watcher : componentAddWatchers[index]) {
                if (watcher.type & GroupWatcherTypes::EnteredGroup && watcher.group.contains(newSignature)) {
                    if (std::find(triggeredPools.begin(), triggeredPools.end(), watcher.pool) == triggeredPools.end()) {
                        triggeredPools.push_back(watcher.pool);
                        watcher.pool->addNew(entity);
                        continue;
                    }
                }
                if (watcher.type & GroupWatcherTypes::ExitedGroup && watcher.group.contains(oldSignature) && !watcher.group.contains(newSignature)) {
                    if (std::find(triggeredPools.begin(), triggeredPools.end(), watcher.pool) == triggeredPools.end()) {
                        triggeredPools.push_back(watcher.pool);
                        watcher.pool->addNew(entity);
                    }
                }
            }
        });
    } 
    
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

        removeEntityIndexFromPool(oldEntityIndex, oldArchetype);
    }

    data->signature = newSignature;
    data->archetype = newArchetypeID;
    data->poolIndex = newEntityIndex;

    return true;
}

void ArchetypalComponentManager::removeComponent(Entity entity, ComponentID component) {
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

    auto oldArchetypeID = data->archetype;

    if (watchedComponentRemoves[component]) {
        SmallVector<ArchetypePool*> triggeredPools;

        Signature newSignature = data->signature;
        Uint8 removeGroupsIndex = watchedComponentRemoveGroupIndices[component];
        auto& groupList = componentRemoveWatchers[removeGroupsIndex];
        for (auto& watcher : groupList) {
            if (watcher.type & GroupWatcherTypes::EnteredGroup) {
                if (watcher.group.contains(newSignature)) {
                    watcher.pool->addNew(entity);
                    triggeredPools.push_back(watcher.pool);
                    continue; // can't enter and exit a group at the same time
                }
            }
            if (watcher.type & GroupWatcherTypes::ExitedGroup) {
                if (watcher.group.contains(oldSignature)) {
                    watcher.pool->addNew(entity);
                    triggeredPools.push_back(watcher.pool);
                }
            }
        }
    }

    auto newArchetypeID = getArchetypeID(data->signature);
    if (newArchetypeID == NullArchetypeID) {
        newArchetypeID = initArchetype(data->signature);
    }

    ArchetypePool* newArchetype = getArchetypePool(newArchetypeID);
    int newEntityIndex = newArchetype->addNew(entity);

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

        removeEntityIndexFromPool(oldEntityIndex, oldArchetype);
    }

    data->archetype = newArchetypeID;
    data->poolIndex = newEntityIndex;
} 

void ArchetypalComponentManager::deleteEntity(Entity entity) {
    if (entity.Null()) return;

    const EntityData* data = entityData.lookup(entity.id);
    if (!data || (entity.version != data->version)) {
        // entity isn't around anyway
        return;
    }

    ArchetypePool* pool = &pools[data->archetype];

    Signature watchedRemoves = pool->signature() & watchedComponentRemoves;
    if (watchedRemoves.any()) {
        SmallVector<ArchetypePool*> triggeredPools;
        watchedRemoves.forEachSet([this, entity, pool, &triggeredPools](ComponentID component){
            auto index = watchedComponentRemoveGroupIndices[component];
            for (auto& watcher : componentRemoveWatchers[index]) {
                if (watcher.type & GroupWatcherTypes::ExitedGroup && watcher.group.contains(pool->signature())) {
                    if (std::find(triggeredPools.begin(), triggeredPools.end(), watcher.pool) == triggeredPools.end()) {
                        watcher.pool->addNew(entity);
                        triggeredPools.push_back(watcher.pool);
                    }
                }
            }
        });
        int oldIndex = data->poolIndex;
    } 

    removeEntityIndexFromPool(data->poolIndex, pool);
    entityData.remove(entity.id);
    unusedEntities.push(entity);
}

ArchetypalComponentManager::ArchetypeID ArchetypalComponentManager::initArchetype(Signature signature) {
    assert(!archetypes.contains(signature));
    Archetype archetype = makeArchetype(signature, componentInfo);

    pools.push_back(archetype);
    archetypes.insert(signature, pools.size()-1);
    return pools.size()-1;
}

void ArchetypalComponentManager::destroy() {
    unusedEntities.destroy();
    archetypes.destroy();
    entityData.destroy();
}

}