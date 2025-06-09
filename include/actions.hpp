#ifndef ACTIONS_INCLUDED
#define ACTIONS_INCLUDED

#include <functional>
#include "GUI/ecs-gui.hpp"

struct Game;

struct GameAction {
    std::function<void(Game* g)> function;
    int type;
};

using GuiAction = void(*)(Game* g, GUI::Element e);
inline GameAction guiActionToGameAction(GuiAction function, GUI::Element e) {
    return {
        [e, function](Game* g){ function(g, e); },
        0
    };
}

namespace GUI {

    void selectHotbarSlot(Game* g, GUI::Element e);

    void makeTheSkyBlue(Game* g, GUI::Element e);

}

#endif