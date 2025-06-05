#ifndef GUI_ELEMENTS_INCLUDED
#define GUI_ELEMENTS_INCLUDED

#include "components.hpp"
#include "ecs-gui.hpp"
#include "rendering/text.hpp"
#include "rendering/context.hpp"
#include "PlayerControls.hpp"
#include "actions.hpp"

namespace GUI {

inline Element boxElement(GuiManager& gui, Box box, SDL_Color backgroundColor) {
    auto e = gui.newElement(ElementTypes::Normal, gui.screen);
    gui.addComponent(e, EC::ViewBox{box});
    gui.addComponent(e, EC::Background({backgroundColor}));
    gui.addComponent(e, EC::Border{.color = {155,255,255,255}, .strokeIn = Vec2(2.0f)});
    gui.addComponent(e, EC::Hover{
        .changeColor = true,
        .newColor = {200, 50, 50, 255},
        .onHover = nullptr
    });
    return e;
}

inline Element funButton(GuiManager& gui, Vec2 pos, Vec2 size, SDL_Color backgroundColor, TextAlignment textAlign, GuiAction onClick) {
    auto e = gui.newElement(ElementTypes::Normal, gui.screen);
    gui.addComponent(e, EC::ViewBox{Box{pos, size}});
    gui.addComponent(e, EC::Background({backgroundColor}));
    gui.addComponent(e, EC::Border{.color = {255,255,255,255}, .strokeIn = Vec2(2.0f)});
    gui.addComponent(e, EC::Button{.onClick = onClick});
    gui.addComponent(e, EC::Text({
        "Do something!",
        TextFormattingSettings{.align = textAlign},
        TextRenderingSettings{
            .color = {255,255,255,255},
            .scale = Vec2(0.3f)
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