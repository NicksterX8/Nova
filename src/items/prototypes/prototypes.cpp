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


}
}