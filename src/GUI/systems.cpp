#include "GUI/systems/basic.hpp"
#include "Game.hpp"

using namespace GUI;
using namespace Systems;

void DoElementUpdatesJob::Execute(int N) {
        auto update = Get<EC::Update>(N);
        auto element = GetEntity(N);
        for (auto action : update.update) {
            action->operator()(game, game->gui.manager, element);
        } 
}