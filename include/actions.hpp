#ifndef ACTIONS_INCLUDED
#define ACTIONS_INCLUDED

#include <functional>
#include "ECS/Entity.hpp"

namespace GUI {
    using Element = ECS::Entity;
    struct GuiManager;
}

struct Game;

struct GameAction {
    std::function<void(Game* g)> function;
    int type;
};

// using GuiAction = std::function<void(Game*, GUI::GuiManager&, GUI::Element)>;
using GuiAction = void(*)(Game* g, GUI::GuiManager&, GUI::Element e);
GameAction guiActionToGameAction(GuiAction function, GUI::Element e);

namespace GUI {

    void selectHotbarSlot(Game* g, GuiManager&, GUI::Element e);

    void makeTheSkyBlue(Game* g, GuiManager&, GUI::Element e);

}

#endif