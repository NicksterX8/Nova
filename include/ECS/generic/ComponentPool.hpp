#ifndef GENERIC_COMPONENT_POOL_INCLUDED
#define GENERIC_COMPONENT_POOL_INCLUDED

#include "global.hpp"
#include "memory.hpp"
#include "llvm/ArrayRef.h"
#include "My/SparseSets.hpp"
#include "utils/common-macros.hpp"

#include "items/Inventory.hpp"

namespace SECS {

using ComponentDestructor = std::function<void(void* components, int count)>;
using ComponentMoveCopyConstructor = std::function<void(void* oldComponents, void* newComponents, int count)>;
using ComponentMoveAssignmentConstructor = ComponentMoveCopyConstructor;

// 65000 - 16 unsigned. 32000 - 16 signed

template<typename EntityID, EntityID MaxEntityID, EntityID NullEntityID>
struct ComponentPool : My::GenericDenseSparseSet<EntityID, MaxEntityID> {
private:
    using Base = My::GenericDenseSparseSet<EntityID, MaxEntityID>;
public:
    using SizeT = Sint32;

    ComponentPool() = default;

    ComponentPool(SizeT componentTypeSize)
    : Base(componentTypeSize) {

    }

    ComponentPool(SizeT componentTypeSize, SizeT startCapacity)
    : Base(componentTypeSize, startCapacity) {

    }

    // Destroy the component pool, deallocating all heap allocations
    void destroy() {
        Base::destroy();
    }

    void* get(EntityID entity) const {
        return Base::lookup(entity);
    }

    // returns true when the component is added, false otherwise
    bool add(EntityID entity, void* value = nullptr) {
        // error checking
        if (UNLIKELY(entity == NullEntityID)) {
            return false; // cant add to null entities
        }
        if (UNLIKELY(Base::contains(entity))) { 
            return true; // entity already has this component
        }

        Base::insert(entity, value);
        return true;
    }

    // entity has the component
    bool has(EntityID entity) {
        return Base::contains(entity);
    }

    // returns true if the entity was removed, false if not
    bool remove(EntityID entity) {
        if (Base::contains(entity)) {
            Base::remove(entity);
            return true;
        }
        return false;
    }

    SizeT size() const {
        return Base::size;
    }

    void reallocate(Sint32 newCapacity) {
        Base::reallocate(newCapacity);
    }
};

/*
template<typename EntityID, EntityID MaxEntityID>
struct 
alignas(64)
ComponentPoolTemplate {
    using SizeT = Sint32;

    size_t componentSize;
    SizeT size; // how many used components exist in the components array
    SizeT capacity; // number of reserved component memory spaces

    Component* components; // array of components
    EntityID* componentOwners; // array of components, same size as 'components'
    SizeT* entityComponentSet; // array indexable by entity id to get the index of the component owned by that entity
    
    const char* name; // human readable name of component type
    ComponentID id;
protected:
    static constexpr SizeT NullIndex = -1;
public:

    static ComponentPool Init(ComponentID componentID, size_t componentSize, Sint32 startCapacity) {
        if (componentSize != 0) {
            if (startCapacity < 0) startCapacity = 0;
            auto self = ComponentPool{
                .componentSize = componentSize,
                .size = 0,
                .capacity = startCapacity,
                .components = (Component*)malloc(startCapacity * componentSize),
                .componentOwners = (EntityID*)malloc(startCapacity * sizeof(EntityID)),
                .entityComponentSet = (Sint32*)malloc(MaxEntityID * sizeof(Sint32)),
                .name = NULL,
                .id = componentID
            };
            if (!self.components || !self.componentOwners || !self.entityComponentSet) {
                LogCrash(CrashReason::MemoryFail, "Failed to allocate memory for component pool %d!", componentID);
            }
            for (Uint32 c = 0; c < MAX_ENTITIES; c++) {
                self.entityComponentSet[c] = NullIndex;
            }
            return self;
        }
        return ComponentPool{
            .componentSize = 0,
            .size = 0,
            .capacity = 0,
            .components = NULL,
            .componentOwners = NULL,
            .entityComponentSet = NULL,
            .name = NULL,
            .id = componentID
        };
    }

    // Destroy the component pool, deallocating all heap allocations
    void destroy() {
        free(components);
        free(componentOwners);
        free(entityComponentSet);
    }

    Component* atIndex(Uint32 index) const {
        char* componentsData = (char*)components;
        return (Component*)(&componentsData[index * componentSize]);
    }

    #define ASSERT_VALID_ENTITY

    Component* get(EntityID entity) const {
        assert(entity < MaxEntityID && "Entity id too high!");

        auto componentIndex = entityComponentSet[entity];

        // this is optimized to just check if index is greater than size rather than two separate checks
        if (componentIndex >= 0 && componentIndex < size) {
            return atIndex(componentIndex);
        }

        LogError("Failed to get component %s for entity %u. Component index: %d.", this->name, entity, componentIndex);
        return NULL;
    }

    // returns 0 on success, -1 on failure.
    int add(EntityID entity) {
        // error checking
        if (UNLIKELY(entity >= NULL_ENTITY_ID)) {
            LogError("ComponentPool(%s)::add : Entity is NULL!", name);
            return -1;
        }
        if (UNLIKELY(entityComponentSet[entity] != ECS_NULL_INDEX)) {
            LogError("ComponentPool(%s)::add : Entity already has this component! Entity: %u", name, entity);
            return -1;
        }

        if (UNLIKELY(size >= capacity)) {
            // double in size every time it runs out of space
            reallocate(MAX(size+1, capacity*2));
        }

        entityComponentSet[entity] = size;
        componentOwners[size] = entity;
        ++size;
        return 0;
    }

    // returns 0 on success, -1 on failure.
    int remove(EntityID entity) {
        Uint32 entityComponentIndex = entityComponentSet[entity];
        if (entityComponentIndex > size) {
            LogError("ComponentPool(%s)::remove : Failed to remove entity from component pool. Entity: %u, component index: %u, size: %u", name, entity, entityComponentSet[entity], size);
            return -1;
        }

        Uint32 lastComponentIndex = size-1;
        EntityID lastComponentOwner = componentOwners[lastComponentIndex];
        if (lastComponentOwner >= NULL_ENTITY_ID) {
            LogError("ComponentPool(%s)::remove : Last component owner is NULL while removing entity %u!", name, entity);
            return -1;
        }
        // dont need to switch data around if the entity is the last entity.
        // Also it would be undefined behavior to copy something to itself
        if (lastComponentOwner != entity) {
            void* entityComponent = atIndex(entityComponentIndex); // the component owned by the entity being removed
            void* last = atIndex(lastComponentIndex); // last component in the array
            memcpy(entityComponent, last, componentSize);
            
            // move the entity owner over to reflect the new position of the (formerly) last component
            // update the address to show it the last component's new position
            entityComponentSet[lastComponentOwner] = entityComponentIndex;
            componentOwners[entityComponentIndex] = lastComponentOwner;
        }
        
        // update it to be empty
        entityComponentSet[entity] = ECS_NULL_INDEX;
        componentOwners[lastComponentIndex] = NULL_ENTITY_ID;
        size--;
        return 0;
    }

    bool reallocate(Sint32 newCapacity) {
        assert(newCapacity >= 0 && "can't have negative component pool capacity");
        newCapacity = MAX(size, newCapacity); // capacity can't be smaller than size
        // skip all the junk below if reallocating to 0
        if (newCapacity == 0 && capacity > 0) {
            free(components);
            free(componentOwners);
            capacity = 0;
            size = 0;
        }

        components = (Component*)Realloc<void>(components,      (size_t)newCapacity * componentSize);
        componentOwners =    Realloc<EntityID>(componentOwners, (size_t)newCapacity);
        
        capacity = newCapacity;
        return true;
    }
};

void inline foiasdjk() {
    //(void)sizeof(SECS::GenericDenseSparseSet<size_t, 4888879204983>::Index);
}
*/

}

#endif