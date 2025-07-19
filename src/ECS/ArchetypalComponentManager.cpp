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
    unusedEntities = My::Vec<Entity>::WithCapacity(1000);
    unusedEntities.require(1000);
    for (int i = 0; i < 1000; i++) {
        unusedEntities[i].id = 1000 - i - 1;
        unusedEntities[i].version = 0;
    }
}

Entity ArchetypalComponentManager::newEntity(Uint32 prototype) {
    Entity entity = unusedEntities.popBack();
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

void ArchetypalComponentManager::removeEntityIndexFromPool(int index, ArchetypePool* pool) {
    ArchetypePool::EntityMoved movedEntity0,movedEntity1;
    pool->remove(index, &movedEntity0, &movedEntity1);
    if (movedEntity0.entity.NotNull()) {
        EntityData* data0 = entityData.lookup(movedEntity0.entity.id);
        data0->poolIndex = movedEntity0.newIndex;
        if (movedEntity1.entity.NotNull()) {
            EntityData* data1 = entityData.lookup(movedEntity1.entity.id);
            data1->poolIndex = movedEntity1.newIndex;
        }
    }
}

void* ArchetypalComponentManager::addComponent(Entity entity, ComponentID component, const void* initializationValue) {
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

        removeEntityIndexFromPool(oldEntityIndex, oldArchetype);
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

bool ArchetypalComponentManager::addSignature(Entity entity, Signature components) {
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

        removeEntityIndexFromPool(oldEntityIndex, oldArchetype);
    }

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