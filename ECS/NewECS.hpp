#ifndef ECS_COMPONENT_POOL_MANAGER_INCLUDED
#define ECS_COMPONENT_POOL_MANAGER_INCLUDED

#include <vector>
#include "ComponentPool.hpp"
#include "ECS.hpp"

class ComponentFlagsManager {
    ECS::ComponentFlags* entityComponentFlags;
};

template<class T>
class ComponentPoolManager {
    ComponentPool* pool = NULL;
    ComponentFlags* entityComponentFlags = NULL;
public:
    ComponentPoolManager(ComponentPool* pool, ComponentFlagsManager& componentFlagsManager) {
        entityComponentFlags = componentFlagsManager.entityComponentFlags;
        this->pool = pool;
    }

    T* Get(Entity entity) {
        return static_cast<T*>(pool->get(entity.id));
    }

    int Add(Entity entity, const T& startValue) {
        int code = pool->add(entity.id);
        if (code) {
            return code;
        }
        pool->components[pool->size()-1] = startValue;
        return 0;
    }

    int Remove(Entity entity) {
        if (!EntityExists(entity)) {
            Log.Error("ComponentPoolManager::Remove : Attempted to remove component \"%s\" from a non-existent entity! Entity: %s", entity.DebugStr());
            return -1;
        }

        entityComponentFlags[entity.id].set(getID<T>(), 0);
        return pool->remove(entity.id);
    }
};

#endif