#include "Methods.hpp"

namespace Entities {

template<class C>
static void tryCopyEntityComponent(ECS* ecs, Entity src, Entity dst, const ComponentFlags& signature) {
    if (signature[getID<C>()])
        *ecs->Get<C>(dst) = *ecs->Get<C>(src);
}

template<class... Components>
static void copyEntityComponents(ECS* ecs, Entity src, Entity dst, const ComponentFlags& entitySignature) {
    int dummy[] = {0, (tryCopyEntityComponent<Components>(ecs, src, dst, entitySignature), 0)...};
    (void)dummy;
}

Entity clone(ECS* ecs, Entity entity) {
    Entity cloned = ecs->New();
    ComponentFlags entitySignature = ecs->entityComponents(entity.id);
    ecs->AddSignature<COMPONENTS>(cloned, entitySignature);
    copyEntityComponents<COMPONENTS>(ecs, entity, cloned, entitySignature);
    return cloned;
}

}