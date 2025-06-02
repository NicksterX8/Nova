#ifndef GUI_UPDATE_INCLUDED
#define GUI_UPDATE_INCLUDED

#include "ecs-gui.hpp"
#include "PlayerControls.hpp"
#include "elements.hpp"

namespace GUI {

inline void init(GuiManager& gui) {
    auto box = boxElement(gui, Box{{120.0f, 20.0f}, {300.0f, 300.0f}});
    auto fun = funButton(gui, Actions::MakeTheSkyBlue);
    gui.addComponent(fun, EC::ChildOf{box});
}

inline void update(GuiManager& gui, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;
    
    gui.forEachElement<EC::Hover, EC::ViewBox>([&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* hover = gui.getComponent<EC::Hover>(e);
        Box box = viewbox->absolute;

        bool mouseOnButton = pointInRect(mousePos, box.rect());
        if (mouseOnButton) {
            // do something when mouse goes over element?
        }
    });

    gui.forEachElement<EC::Button, EC::ViewBox>([&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* button = gui.getComponent<EC::Button>(e);
        Box box = viewbox->absolute;

        bool mouseOnButton = pointInRect(mousePos, box.rect());
        if (mouseOnButton) {
            if (mouseClicked) {
                doAction(button->onClick);
            }
        }
    });

}

inline void renderElements(GuiManager& gui, GuiRenderer& renderer, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;

    constexpr int MaxDepth = 16;
    My::Vec< My::Vec<Element> > elementsByDepth(MaxDepth);


    gui.forEachElement<EC::ViewBox>([&](Element e){
        auto boxEc = gui.getComponent<EC::ViewBox>(e);
        if (gui.elementHas<EC::ChildOf>(e)) return;

        boxEc->absolute = boxEc->box;
        boxEc->depth = 0;
        //elementsByDepth[0].push(e);
    });

    // calculate aboslute positions
    gui.forEachElement<EC::ViewBox, EC::ChildOf>([&](Element e){
        auto boxEc = gui.getComponent<EC::ViewBox>(e);
        auto box = boxEc->box;
        auto child = gui.getComponent<EC::ChildOf>(e);

        Vec2 pos = box.min;

        int parentCounter = 0;

        // go up parent chain all the way till the top
        while (child != nullptr) {
            Element parent = child->parent;
            if (parent.Null()) break;

            auto pBoxEc = gui.getComponent<EC::ViewBox>(parent);
            if (pBoxEc) {
                auto pBox = pBoxEc->absolute;
                pos += pBox.min;
                parentCounter++;
            }

            child = gui.getComponent<EC::ChildOf>(parent);
        }

        boxEc->absolute = Box{
            pos,
            box.size
        };
        boxEc->depth = parentCounter;
    });

    // backgrounds
    gui.forEachElement<EC::ViewBox, EC::Background>([&](Element e){
        auto* view = gui.getComponent<EC::ViewBox>(e);
        SDL_Color color = gui.getComponent<EC::Background>(e)->color;

        bool mouseOnButton = pointInRect(mousePos, view->absolute.rect());
        if (mouseOnButton) {
            auto* hover = gui.getComponent<EC::Hover>(e);
            if (hover && hover->changeColor) {
                color = hover->newColor;
            }
        }
        renderer.colorRect(view->absolute.min, view->absolute.max(), color, view->depth);
    });

    // textures
    gui.forEachElement<EC::ViewBox, EC::SimpleTexture>([&](Element e){
        Box entityBox = gui.getComponent<EC::ViewBox>(e)->box;
        auto* texture = gui.getComponent<EC::SimpleTexture>(e);

        Box texBox = {
            entityBox.min + texture->texBox.min,
            texture->texBox.size
        };

        renderer.sprite(texture->texture, texBox.rect());
    });

    // borders
    gui.forEachElement<EC::ViewBox, EC::Border>([&](Element e){
        auto box = gui.getComponent<EC::ViewBox>(e)->absolute;
        auto* border = gui.getComponent<EC::Border>(e);
        renderer.rectOutline(box.min, box.max(), border->color, border->thickness, 0.0f);
    });


    gui.forEachElement<EC::Hover, EC::ViewBox>([&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* hover = gui.getComponent<EC::Hover>(e);
        Box box = viewbox->absolute;

        bool mouseOnButton = pointInRect(mousePos, box.rect());
        if (mouseOnButton) {
            
        }
    });
    
}

}

#endif