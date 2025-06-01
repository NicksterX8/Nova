#ifndef GUI_ELEMENTS_INCLUDED
#define GUI_ELEMENTS_INCLUDED

#include "components.hpp"
#include "ecs-gui.hpp"
#include "rendering/text.hpp"

namespace GUI {

struct GuiElementProto : Prototype {
    GuiElementProto(PECS::PrototypeManager* manager) : Prototype(PECS::New(manager, PrototypeIDs::Normal)) {

    }
};

inline void doSomething() {
    LogInfo("This is a button press!");
}

inline void renderElements(GuiManager& gui, RenderContext& ren) {
    Element elements[10];
    for (int i = 0; i < 10; i++) {
        Element& e = elements[i];
        auto boxEc = getComponent<EC::ViewBox>(e, gui);
        if (!boxEc) continue;
        auto box = boxEc->box;
        auto child = getComponent<EC::Child>(e, gui);
        Vec2 pos = box.min;

        // go up parent chain all the way till the top
        while (1) {
            Element* parent = gui.elements.lookup(child->parent);
            if (parent == nullptr) break;

            auto pBoxEc = getComponent<EC::ViewBox>(e, gui);
            if (pBoxEc) {
                auto pBox = pBoxEc->box;
                pos += pBox.min;
            }

            auto child = getComponent<EC::Child>(*parent, gui);
            if (child == nullptr) break;
        }

        box.min = pos;

        if (e.has<EC::Background>()) {
            ren.guiRenderer.colorRect(box.min, box.max(), getComponent<EC::Background>(e, gui)->color);
        }

        if (e.has<EC::Border>()) {
            auto border = *getComponent<EC::Border>(e, gui);
            ren.guiRenderer.rectOutline(box.min, box.max(), border.color, border.thickness, 0.0f);
        }
        
    }
}

inline void doAction(GUI::EC::Action action) {

}

inline ElementID boxElement(GuiManager& gui, Box box) {
    auto e = makeElement(gui);
    addComponent(e, gui, EC::ViewBox{box});
    return e;
}

inline Element funButton(GuiManager& gui, GUI::EC::Action onClick) {
    auto e = makeElement(gui);
    addComponent(e, gui, EC::ViewBox{Box{{50.0f, 50.0f}, {100.0f, 100.0f}}});
    addComponent(e, gui, EC::Background({{0,0,0,255}}));
    addComponent(e, gui, EC::Border({{255,255,255,255}, 2.0f}));
    addComponent(e, gui, EC::Button(onClick));
    addComponent(e, gui, EC::Text({
        "Do something!",
        TextFormattingSettings(TextAlignment::MiddleCenter),
        TextRenderingSettings({255,255,255,255})
    }));

    return e;
}

namespace Actions {
enum ActionEnum {
    KillEveryone,
    MakeTheSkyBlue
};
}

void init(GuiManager& gui) {
    auto box = boxElement(gui, Box{{20.0f, 20.0f}, {500.0f, 500.0f}});
    auto fun = funButton(gui, Actions::MakeTheSkyBlue);
    addComponent(fun, gui, GUI::EC::Child{&box});
}

void update(GuiManager& gui) {
    gui.forEachEntity<GUI::EC::ViewBox, GUI::EC::Texture>([&](Element element){
        auto* viewBox = GUI::getComponent<GUI::EC::ViewBox>(element, gui);
       
    });

    gui.forEachEntity<GUI::EC::Button>([&](Element element){
        auto* button = GUI::getComponent<GUI::EC::Button>(element, gui);
        if (button->clickedThisFrame) {
            doAction(button->onClick);
            button->clickedThisFrame = false;
        }
    });
}

}

#endif