#include "items/prototypes/prototypes.hpp"
#include "ECS/entities/entities.hpp"
#include "Game.hpp"

namespace items {
namespace Prototypes {

bool Grenade::onUse(Game* g) {
    auto grenade = Entities::Grenade(
        &g->state->ecs,
        g->state->player.get<EC::Position>()->vec2()); 
    Entities::throwEntity(&g->state->ecs, grenade, g->playerControls->mouseWorldPos, 1.5f);
    return true;
}

bool SandGun::onUse(Game* g) {
    auto& ecs = g->state->ecs;
    auto sand = ecs.New("sand particle");
    
    ecs.Add(sand, EC::Render({TextureIDs::Tiles::Sand, RenderLayers::Particles}));
    auto* playerPos = g->state->player.get<EC::Position>();
    ecs.Add(sand, EC::Position(*playerPos));
    ecs.Add(sand, EC::ViewBox({Box{Vec2(-0.5), Vec2(1)}}));
    ecs.Add(sand, EC::CollisionBox({Box{Vec2(-0.5), Vec2(1)}}));
    ecs.Add(sand, EC::Dynamic({playerPos->vec2()}));
    ecs.Add(sand, EC::Motion({g->playerControls->mouseWorldPos, 0.3f}));
    ecs.Add(sand, EC::Dying(3*60));
    return true;
}

}
}