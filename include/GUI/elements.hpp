#ifndef GUI_ELEMENTS_INCLUDED
#define GUI_ELEMENTS_INCLUDED

#include "components.hpp"
#include "ecs-gui.hpp"
#include "rendering/text.hpp"
#include "rendering/context.hpp"
#include "PlayerControls.hpp"

namespace GUI {

namespace ElementTypes {
    enum {
        Normal,
        Epic,
        Count
    };
}

namespace Prototypes {

    struct Normal : Prototype {
        Normal(GECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {

        }
    };

    struct Epic : Prototype {
        Epic(GECS::PrototypeManager& manager) : Prototype(manager.New(ElementTypes::Normal)) {
            
        }
    };

}

inline void makeGuiPrototypes(GuiManager& gui) {
    auto& pm = gui.prototypes;

    auto normal = Prototypes::Normal(pm);
    auto epic = Prototypes::Epic(pm);

    pm.add(normal);
    pm.add(epic);
}

namespace Actions {
enum ActionEnum {
    KillEveryone,
    MakeTheSkyBlue
};
}

inline void doAction(EC::Action action) {
    switch (action) {
    case Actions::MakeTheSkyBlue:
        LogInfo("The sky is blue noe1!");
        break;
    default:
        break;
    }
}

inline Element boxElement(GuiManager& gui, Box box) {
    auto e = gui.newElement(ElementTypes::Normal);
    gui.addComponent(e, EC::ViewBox{box});
    gui.addComponent(e, EC::Background({{150,150,150,255}}));
    gui.addComponent(e, EC::Border({{155,255,255,255}, 2.0f}));
    return e;
}

inline Element funButton(GuiManager& gui, EC::Action onClick) {
    auto e = gui.newElement(ElementTypes::Normal);
    gui.addComponent(e, EC::ViewBox{Box{{110.0f, 50.0f}, {300.0f, 100.0f}}});
    gui.addComponent(e, EC::Background({{0,0,0,255}}));
    gui.addComponent(e, EC::Border({{255,255,255,255}, 2.0f}));
    gui.addComponent(e, EC::Button(onClick));
    gui.addComponent(e, EC::Text({
        "Do something!",
        TextFormattingSettings(TextAlignment::MiddleCenter),
        TextRenderingSettings({255,255,255,255})
    }));
    gui.addComponent(e, EC::Hover({
        true, {50, 50, 250, 255},
        0
    }));

    return e;
}

}

#endif