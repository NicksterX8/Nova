#include "world/EntityWorld.hpp"
#include "world/functions.hpp"

namespace World {

    EntityMaker::EntityMaker(EntityWorld* ecs)
    : ecs(ecs) {
        buf = ecs->getEntityMakerBuffer();
    }

    Entity EntityMaker::make() {
        if (entity.Null()) {
            entity = ecs->New(prototype);
            ecs->AddSignature(entity, components);
            for (auto& componentValue : componentValues) {
                ecs->Set(entity, componentValue.id, &buf[componentValue.valueBufPos]);
            }
        }
        return entity;
    }

    void EntityMaker::clear() {
        entity = NullEntity;
        components = 0;
        prototype = -1;
        bufUsed = 0;
        componentValues.clear();
    }

}