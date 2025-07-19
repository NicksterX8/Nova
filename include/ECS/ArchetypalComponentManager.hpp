#pragma once

#include "ArchetypePool.hpp"

namespace ECS {

struct ArchetypalComponentManager {
    static constexpr size_t MaxEntityID = (1 << 14) - 1;

    using ArchetypeID = Sint16;
    static constexpr ArchetypeID NullArchetypeID = -1;

    SmallVector<ArchetypePool, 0> pools;
    My::Vec<Entity> unusedEntities;
    My::HashMap<Signature, ArchetypeID, SignatureHash> archetypes;
    ComponentInfoRef componentInfo;
    Signature validComponents = {0};

    struct EntityData {
        Signature signature;
        Sint32 prototype;
        Uint32 version;
        Sint16 poolIndex;
        ArchetypeID archetype;
    };

    My::DenseSparseSet<EntityID, EntityData, 
        Uint16, MaxEntityID> entityData;

    void init(ComponentInfoRef componentInfo);

    Entity newEntity(Uint32 prototype);

    void deleteEntity(Entity entity);

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

    void* getComponent(Entity entity, ComponentID component) const;

    void removeEntityIndexFromPool(int index, ArchetypePool* pool);

    // returns true on success, false on failure
    bool addSignature(Entity entity, Signature components);

    void* addComponent(Entity entity, ComponentID component, const void* initializationValue = nullptr);

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

    const ArchetypePool* getArchetypePool(ArchetypeID id) const {
        if (id >= pools.size()) return nullptr;
        return &pools[id];
    }

    ArchetypePool* getArchetypePool(ArchetypeID id) {
        if (id >= pools.size()) return nullptr;
        return &pools[id];
    }
public:

    ArchetypeID initArchetype(Signature signature);

    const ComponentInfo& getComponentInfo(ComponentID component) const {
        return componentInfo.get(component);
    }

    void destroy();
};

}