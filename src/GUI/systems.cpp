#include "GUI/systems/basic.hpp"
#include "Game.hpp"

using namespace GUI;
using namespace Systems;

void DoElementUpdatesJob::Execute(int N) {
    updates[N].update(game, game->gui.manager, elements[N]);
}