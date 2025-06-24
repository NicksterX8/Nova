#include "world/entities/methods.hpp"
#include "GameState.hpp"

namespace World {

namespace Entities {

    void throwEntity(EntityWorld& ecs, Entity entity, Vec2 target, float speed) {
        ecs.Add(entity, EC::Motion(target, speed));
        if (ecs.EntityHas<EC::Rotation>(entity)) {
            float rotation = ecs.Get<EC::Rotation>(entity)->degrees;
            float timeToTarget = target.length() / speed;
            ecs.Add<EC::AngleMotion>(entity, EC::AngleMotion(rotation + timeToTarget * 30.0f, 30.0f));
        }
    }

    Entity findNamedEntity(const char* name, const EntityWorld* ecs) {
        Entity target = NullEntity;
        ecs->ForEach_EarlyReturn([](ECS::Signature components){
            return components[EC::Nametag::ID];
        }, [&](Entity entity){
            auto nametag = ecs->Get<const EC::Nametag>(entity);
            // super inefficient btw
            if (My::streq(name, nametag->name)) {
                target = entity;
                return true;
            }
            return false;
        });

        return target;
    }

    void scaleGuy(GameState* state, Entity guy, float scale) {
        auto view = state->ecs->Get<EC::ViewBox>(guy);
        if (view) {
            auto oldViewbox = view->box;
            view->box.size = view->box.size * scale;
            entityViewboxChanged(state, guy, oldViewbox);
        }
        auto collision = state->ecs->Get<EC::CollisionBox>(guy);
        if (collision) {
            Vec2 size = collision->box.size;
            Vec2 newSize = size * scale;
            collision->box.min *= scale;
            collision->box.size *= scale;
        }
    }

}

}