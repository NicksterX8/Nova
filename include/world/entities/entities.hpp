#ifndef ENTITIES_INCLUDED
#define ENTITIES_INCLUDED

#include "ECS/ECS.hpp"
#include "world/components/components.hpp"
#include "rendering/textures.hpp"
#include "world/EntityWorld.hpp"
#include "utils/vectors_and_rects.hpp"
#include "prototypes/prototypes.hpp"

namespace World {

using ExplosionCreateFunction = Entity(EntityWorld*, Vec2);

namespace Entities {

    #define MARK_START_ENTITY_CREATION(ecs) {ecs->StartDeferringEvents();}
    #define MARK_END_ENTITY_CREATION(ecs) {ecs->StopDeferringEvents();}

    inline EntityMaker& makeMurderer(EntityWorld* ecs, Vec2 pos) {
        auto& e = ecs->startMakingEntity(PrototypeIDs::Player);
        e.setPos(pos);
        e.Add(EC::Render(TextureIDs::Player, RenderLayers::Player));
        e.Add(EC::CollisionBox(Vec2(0.05f), Vec2(0.9f)));
        e.Add<EC::Rotation>({0.0f});
        e.Add<EC::Health>({300});
        return e;
    }

    Entity ThrownGrenade(EntityWorld* ecs, Vec2 position, Vec2 target);

    struct TransportBelt : Entity {
        TransportBelt(EntityBuilder ecs, Vec2 position) {
            auto& e = ecs.New(PrototypeIDs::TransportBelt);
            e.Add<EC::Health>({100.0f});
            e.Add<EC::Position>(position);
            e.Add<EC::ViewBox>({Box{Vec2(0), Vec2(1)}});
            e.Add<EC::Render>(EC::Render(TextureIDs::Buildings::TransportBelt, RenderLayers::Buildings));
            e.Add<EC::Rotation>({0.0f});
            e.Add<EC::Rotatable>(EC::Rotatable(0.0f, 90.0f));
            e.Add<EC::Transporter>({0.15f});
            e.make(this);
        }
    };
}

}

#endif