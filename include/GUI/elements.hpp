#ifndef GUI_ELEMENTS_INCLUDED
#define GUI_ELEMENTS_INCLUDED

#include "components.hpp"
#include "ecs-gui.hpp"
#include "rendering/text/text.hpp"
#include "rendering/context.hpp"
#include "PlayerControls.hpp"
#include "actions.hpp"

namespace GUI {

inline void addView(GuiManager& gui, Element element, Box box, GUI::RenderLevel level = GUI::RenderLevel::Null, bool visible = true) {
    gui.addComponent(element, EC::ViewBox{.box = box, .level = level, .visible = visible});
    gui.addComponent(element, EC::DisplayBox{});
}

inline void addUpdate(GuiManager& gui, Element element, const NewGuiAction& update) {
    EC::Update* updateEc = gui.getComponent<EC::Update>(element);
    if (!updateEc) {
        gui.addComponent(element, EC::Update{.update = {update}});
        return;
    } else {
        updateEc->update.push_back(update);
    }
}

inline void hideIf(GuiManager& gui, Element element, std::function<bool(Game* game, GuiManager& manager, Element toHide)> shouldHide) {
    addUpdate(gui, element, [shouldHide](Game* game, GuiManager& gui, Element element){
        if (shouldHide(game, gui, element)) {
            gui.hideElement(element);
        } else {
            gui.unhideElement(element);
        }
    });
}

inline Element boxElement(GuiManager& gui, Box box, SDL_Color backgroundColor) {
    auto e = gui.newElement(ElementTypes::Normal, gui.screen);
    addView(gui, e, box);
    gui.addComponent(e, EC::Background({backgroundColor}));
    gui.addComponent(e, EC::Border{.color = {155,255,255,255}, .strokeIn = Vec2(2.0f)});
    gui.addComponent(e, EC::Hover{
        .changeColor = true,
        .newColor = {200, 50, 50, 255},
        .onHover = nullptr
    });
    return e;
}

inline Element funButton(GuiManager& gui, Vec2 pos, Vec2 size, SDL_Color backgroundColor, TextAlignment textAlign, const NewGuiAction& onClick) {
    auto e = gui.newElement(ElementTypes::Normal, gui.screen);
    addView(gui, e, {pos, size});
    gui.addComponent(e, EC::Background({backgroundColor}));
    gui.addComponent(e, EC::Border{.color = {255,255,255,255}, .strokeIn = Vec2(2.0f)});
    gui.addComponent(e, EC::Button{{onClick}});
    gui.addComponent(e, EC::Text({
        "Do something!",
        TextFormattingSettings{.align = textAlign},
        TextRenderingSettings{
            .color = {255,255,255,255},
            .scale = 0.3f
        }
    }));
    gui.addComponent(e, EC::Hover({
        true, {50, 50, 250, 255},
        0
    }));

    return e;
}

}

#endif