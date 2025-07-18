#include "GUI/systems/basic.hpp"
#include "Game.hpp"

using namespace GUI;
using namespace Systems;

void DoElementUpdatesJob::Execute(int N, EntityArray elements, ComponentArray<const EC::Update> updates) {
    for (auto action : updates[N].update) {
        action->operator()(game, game->gui.manager, elements[N]);
    } 
}