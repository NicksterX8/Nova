#include "world/EntityWorld.hpp"
#include "world/functions.hpp"
#include "world/components/components.hpp"

namespace World {

    // EntityMaker::EntityMaker(EntityWorld* ecs)
    // : ecs(ecs) {
    //     buf = ecs->getEntityMakerBuffer();
    // }

    // EntityMaker& EntityMaker::operator=(const EntityMaker& other) {
    //     this->ecs = other.ecs;
    //     this->buf = other.ecs->getEntityMakerBuffer();
    //     return *this;
    // }

    // Entity EntityMaker::make() {
    //     if (entity.Null()) {
    //         entity = ecs->New(prototype);
    //         ecs->AddSignature(entity, components);
    //         for (auto& componentValue : componentValues) {
    //             ecs->Set(entity, componentValue.id, &buf[componentValue.valueBufPos]);
    //         }
    //     }
    //     return entity;
    // }

    // void EntityMaker::clear() {
    //     entity = NullEntity;
    //     components = 0;
    //     prototype = -1;
    //     bufUsed = 0;
    //     componentValues.clear();
    // }

    void EntityWorld::init(EntityWorld* pointerToThis) {
        using namespace EC;
        using namespace EC::Proto;
        static const auto infoList = ECS::getComponentInfoList<WORLD_COMPONENT_LIST>();
        Base::init(infoList, World::Entities::PrototypeIDs::Count);
    }

    ECS::EntityVersion EntityWorld::GetEntityVersion(ECS::EntityID id) const {
        assert(id <= NullEntity.id);
        auto* data = Base::components.getEntityData(id);
        if (data)
            return data->version;
        return NullEntity.version;
    }

    void EntityWorld::Set(Entity entity, ECS::ComponentID componentID, void* value) {
        assert(value);
        void* component = Base::getComponent(entity, componentID);
        if (component) {
            memcpy(component, value, getComponentSize(componentID));
        } else {
            LogError("Failed to set component, it could not be found.");
        }
    }

}