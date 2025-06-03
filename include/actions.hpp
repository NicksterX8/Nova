#ifndef ACTIONS_INCLUDED
#define ACTIONS_INCLUDED

#include <functional>
#include "ECS/generic/ArchetypePool.hpp"

struct Game;

struct GameAction {
    std::function<void(Game* g)> function;
    int type;
};

using GuiAction = void(*)(Game* g, GECS::Element e);
inline GameAction guiActionToGameAction(GuiAction function, GECS::Element e) {
    return {
        [e, function](Game* g){ function(g, e); },
        0
    };
}

namespace GUI {

    void selectHotbarSlot(Game* g, GECS::Element e);

    void makeTheSkyBlue(Game* g, GECS::Element e);

}

#endif