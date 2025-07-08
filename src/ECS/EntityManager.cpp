#include "ECS/EntityManager.hpp"

namespace ECS {

ComponentID EntityManager::getComponentIdFromName(const char* name) const {
    for (ComponentID id = 0; id < components.componentInfo.size(); id++) {
        if (My::streq(components.getComponentInfo(id).name, name)) {
            return id;
        }
    }
    return NullComponentID;
}

}