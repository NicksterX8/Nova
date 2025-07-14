#ifndef ACTIONS_INCLUDED
#define ACTIONS_INCLUDED

#include <functional>
#include "ECS/Entity.hpp"
#include "ADT/TinyPtrVectorPod.hpp"

namespace GUI {
    using Element = ECS::Entity;
    struct GuiManager;
}

struct Game;

struct GameAction {
    std::function<void(Game* g)> function;
    int type;
};

using GuiAction = void(*)(Game* g, GUI::GuiManager&, GUI::Element e);
using NewGuiAction = std::function<void(Game*, GUI::GuiManager&, GUI::Element)>;
struct NewGuiActionList : private TinyPtrVectorPOD<NewGuiAction*> {
    using TinyPtrVectorPOD<NewGuiAction*>::begin;
    using TinyPtrVectorPOD<NewGuiAction*>::end;

    NewGuiActionList() = default;

    NewGuiActionList(const NewGuiAction& action) {
        push_back(action);
    }

    NewGuiActionList(GuiAction action) {
        push_back(action);
    }

    void push_back(const NewGuiAction& action) {
        TinyPtrVectorPOD<NewGuiAction*>::push_back(new NewGuiAction(action));
    }

    void push_back(GuiAction action) {
        TinyPtrVectorPOD<NewGuiAction*>::push_back(new NewGuiAction(action));
    }

    void destroy() {
        for (auto action : *this) {
            delete action;
        }
        TinyPtrVectorPOD<NewGuiAction*>::destroy();
    }
};
GameAction guiActionToGameAction(GuiAction function, GUI::Element e);
GameAction guiActionToGameAction(NewGuiAction* function, GUI::Element e);

namespace GUI {

    void selectHotbarSlot(Game* g, GuiManager&, GUI::Element e);

    void makeTheSkyBlue(Game* g, GuiManager&, GUI::Element e);

}

#endif