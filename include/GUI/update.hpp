#ifndef GUI_UPDATE_INCLUDED
#define GUI_UPDATE_INCLUDED

#include "ecs-gui.hpp"
#include "PlayerControls.hpp"
#include "elements.hpp"

namespace GUI {

inline Element buildConsole(GuiManager& gui) {
    float scale = 1;



    auto console = gui.newElement(ElementTypes::Normal, gui.screen);
    gui.addComponent(console, EC::Background{});


    auto terminal = gui.newElement(ElementTypes::Normal, console);


    auto log = gui.newElement(ElementTypes::Normal, console);

    return console;
}

inline Element buildHotbar(GuiManager& gui) {
    float scale = 1;

    const float opacity = 0.5;
    Uint8 alpha = opacity * 255;
    unsigned int numSlots = 9;
    float width = 600 * scale;

    float slotSize = width / numSlots;
    float borderSize = 4 * scale;

    float borderHeight = slotSize + borderSize*2;
    float borderWidth = width + borderSize*2;

    float horizontalMargin = 100;

    SDL_Color backgroundColor = {200, 100, 100, alpha};
    SDL_Color borderColor = {60, 60, 60, alpha};
    SDL_Color slotBackgroundColor = {45, 45, 45, alpha};
    SDL_Color slotHoverColor = {15, 15, 15, alpha};

    SDL_FRect barRect = {
        horizontalMargin,
        200,
        borderWidth,
        borderHeight 
    };
    
    #define HOTBAR_FONT "Gui"
    //auto font = Fonts->get(HOTBAR_FONT);

    auto bar = gui.newElement(ElementTypes::Normal, gui.screen);
    
    EC::Name name;
    strcpy(name.name, "hotbar");
    gui.addComponent(bar, name);
    gui.addComponent(bar, EC::ViewBox{*rectAsBox(&barRect)});
    gui.addComponent(bar, EC::SizeConstraint{.maxSize = {800 * scale, INFINITY}});
    gui.addComponent(bar, EC::AlignmentConstraint{.alignment = TextAlignment::BottomCenter});
    gui.addComponent(bar, EC::Background({backgroundColor}));
    gui.addComponent(bar, EC::Border({borderColor, borderSize}));

    // slots
    float innerMargin = 2 * scale;

    float offset = borderSize;
    
    for (int s = 0; s < numSlots; s++) {
        auto slot = gui.newElement(ElementTypes::Normal, bar);
        EC::Name name;
        snprintf(name.name, 64, "hotbar-slot-%d", s);
        gui.addComponent(slot, name);
        gui.addComponent(slot, EC::Numbered{s});

        Box hotbarSlot = {
            {offset, borderSize},
            Vec2{slotSize}
        };

        Box innerSlot = {
            Vec2(innerMargin),
            hotbarSlot.size - Vec2(innerMargin*2) 
        };

        offset += slotSize;

        gui.addComponent(slot, EC::ViewBox{hotbarSlot});
        gui.addComponent(slot, EC::SimpleTexture{0, innerSlot});
        gui.addComponent(slot, EC::Background{slotBackgroundColor});
        gui.addComponent(slot, EC::Hover{true, slotHoverColor, 0});
        float textScale = scale/2.0f;
        gui.addComponent(slot, EC::Text({
            {'\0'},
            TextFormattingSettings(TextAlignment::BottomLeft),
            TextRenderingSettings({255,255,255,255}, Vec2(textScale))
        }));
        gui.addComponent(slot, EC::Button{selectHotbarSlot});
    }

    return bar;
}

inline void init(GuiManager& gui) {
    /*
    auto box = boxElement(gui, Box{{120.0f, 20.0f}, {300.0f, 300.0f}}, {0, 0, 255, 255});
    auto box2 = boxElement(gui, Box{{20.0f, 20.0f}, {250.0f, 250.0f}}, {0, 255, 0, 255});
    gui.addComponent(box2, EC::ChildOf{box});
    auto box3 = boxElement(gui, Box{{20.0f, 20.0f}, {210.0f, 210.0f}}, {255, 0, 0, 255});
    gui.addComponent(box3, EC::ChildOf{box2});
    auto fun = funButton(gui, {30, 20}, {100, 300}, SDL_Color{0,100,255,255}, TextAlignment::BottomLeft, Actions::MakeTheSkyBlue);
    auto fun2 = funButton(gui, {500, 20}, {300, 300}, SDL_Color{0,100,255,255}, TextAlignment::MiddleCenter, Actions::MakeTheSkyBlue);
    auto fun3 = funButton(gui, {30, 500}, {300, 300}, SDL_Color{0,100,255,255}, TextAlignment::TopRight, Actions::MakeTheSkyBlue);
    gui.addComponent(fun, EC::ChildOf{box3});
    */

    auto bar = buildHotbar(gui);
    //auto halfBox = boxElement(gui, {Vec2(0.0f), Vec2(50.0f)}, {150, 0, 0, 255});
    //gui.addComponent(halfBox, EC::SizeConstraint{.relativeSize = Vec2(0.5, 0.2)});
}

inline void bufferAction(GuiAction guiAction, GUI::Element e, std::vector<GameAction>* gameActions) {
    gameActions->push_back(guiActionToGameAction(guiAction, e));
}

inline std::vector<GameAction> update(GuiManager& gui, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;

    g.mouseClickedOnGui = false;

    std::vector<GameAction> actions;
    
    gui.forEachElement([&](auto signature){
        return signature[EC::ViewBox::ID] && signature[EC::Hover::ID];
    }, [&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* hover = gui.getComponent<EC::Hover>(e);
        Box box = viewbox->absolute;

        bool mouseOnButton = pointInRect(mousePos, box.rect());
        if (mouseOnButton) {
            // do something when mouse goes over element?
        }
    });

    return actions;
}

inline void buildFromTree(GuiTreeNode* node, GuiManager& gui, int level) {
    Element parent = node->e;
    if (parent.Null()) return;

    Box parentBox = gui.getComponent<EC::ViewBox>(parent)->absolute;
    
    for (int i = 0; i < node->childCount; i++) {
        Element e = node->children[i].e;
        auto* viewEc = gui.getComponent<EC::ViewBox>(e);
        Box childBox = viewEc->box;
        auto* sizeConstraint = gui.getComponent<EC::SizeConstraint>(e);
        if (sizeConstraint && sizeConstraint->relativeSize.x != INFINITY && sizeConstraint->relativeSize.y != INFINITY) {
            childBox.size = sizeConstraint->relativeSize * parentBox.size;
        }
        auto* alignmentConstraint = gui.getComponent<EC::AlignmentConstraint>(e);
        Vec2 offset = {0, 0};
        if (alignmentConstraint) {
            Vec2 margin = parentBox.size - childBox.size;
            offset.x = (int)alignmentConstraint->alignment.horizontal * 0.5f * margin.x;
            offset.y = (2 - (int)alignmentConstraint->alignment.vertical) * 0.5f * margin.y;
            childBox.min = offset;
        }
        childBox.min += parentBox.min;
        
        viewEc->level = level;
        viewEc->absolute = childBox;

        buildFromTree(&node->children[i], gui, ++level);
    }
}

inline void renderElements(GuiManager& gui, GuiRenderer& renderer, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;

    /*
    gui.forEachElement([&](auto signature){
        return signature[EC::ViewBox::ID] && !signature[EC::ChildOf];
    }, [&](Element e){
        auto boxEc = gui.getComponent<EC::ViewBox>(e);

        boxEc->absolute = boxEc->box;
        boxEc->level = 0;
    });
    */

    auto screenBox = Box{Vec2(0), renderer.options.size};
    gui.setComponent(gui.screen, EC::ViewBox{screenBox, screenBox});

    int level = 1;
    buildFromTree(&gui.root, gui, level);

    Element slot = gui.getNamedElement("hotbar-slot-0");
    auto* slotView = gui.getComponent<EC::ViewBox>(slot);

    // calculate aboslute positions
    /*
    gui.forEachElement<EC::ViewBox, EC::ChildOf>([&](Element e){
        auto boxEc = gui.getComponent<EC::ViewBox>(e);
        auto box = boxEc->box;
        auto child = gui.getComponent<EC::ChildOf>(e);

        Vec2 pos = box.min;
        Vec2 size = box.size;

        int parentCounter = 0;

        // go up parent chain all the way till the top
        while (child != nullptr) {
            Element parent = child->parent;
            if (parent.Null()) break;

            auto pBoxEc = gui.getComponent<EC::ViewBox>(parent);
            if (pBoxEc) {
                auto pBox = pBoxEc->box;
                pos += pBox.min;
                auto* sizeConstraint = gui.getComponent<EC::SizeConstraint>(e);
                if (sizeConstraint) {
                   size = sizeConstraint->relativeSize * pBoxEc->box.size;
                }

                parentCounter++;
            }

            child = gui.getComponent<EC::ChildOf>(parent);
        }

        boxEc->absolute = Box{
            pos,
            size
        };
        boxEc->level = parentCounter;
        if (boxEc->level >= GuiNumLevels) {
            boxEc->level = GuiNumLevels-1;
        }
    });
    

    gui.forEachElement<EC::ViewBox, EC::SizeConstraint>([&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);
        auto* sizeConstraint = gui.getComponent<EC::SizeConstraint>(e);
        Element parent = gui.getComponent<EC::ChildOf>(e)->parent;
        auto* parentBox = gui.getComponent<EC::ViewBox>(parent);

        viewbox->box.size = sizeConstraint->relativeSize * parentBox->box.size;
    });
    */


    int hoveredLevel = -1;
    Element hoveredElement = GECS::NullElement;

    gui.forEachElement([&](auto signature){ 
        return signature[EC::ViewBox::ID] && !signature[EC::Hidden::ID];
    }, [&](Element e){
        auto* viewbox = gui.getComponent<EC::ViewBox>(e);

        bool mouseOnButton = pointInRect(mousePos, viewbox->absolute.rect());
        if (mouseOnButton) {
            if (viewbox->level > hoveredLevel) {
                hoveredLevel = viewbox->level;
                hoveredElement = e;
            }
        }
    });

    gui.hoveredElement = hoveredElement;

    /* Elements are rendered based on their level
     * Parts of elements are rendered according to the order the forEach calls are made here
     */

    // backgrounds
    gui.forEachElement<EC::ViewBox, EC::Background>([&](Element e){
        auto* view = gui.getComponent<EC::ViewBox>(e);
        SDL_Color color = gui.getComponent<EC::Background>(e)->color;

        if (e == hoveredElement) {
            auto* hover = gui.getComponent<EC::Hover>(e);
            if (hover && hover->changeColor) {
                color = hover->newColor;
            }
        }
        renderer.colorRect(view->absolute.min, view->absolute.max(), color, view->level);
    });

    // textures
    gui.forEachElement<EC::ViewBox, EC::SimpleTexture>([&](Element e){
        auto* view = gui.getComponent<EC::ViewBox>(e);
        Box entityBox = view->absolute;
        auto* texture = gui.getComponent<EC::SimpleTexture>(e);

        Box texBox = {
            entityBox.min + texture->texBox.min,
            texture->texBox.size
        };

        renderer.sprite(texture->texture, texBox.rect(), view->level);
    });

    // borders
    gui.forEachElement<EC::ViewBox, EC::Border>([&](Element e){
        auto view = gui.getComponent<EC::ViewBox>(e);
        auto box = view->absolute;
        auto* border = gui.getComponent<EC::Border>(e);
        renderer.rectOutline(box.min, box.max(), border->color, border->thickness, 0.0f, view->level);
    });

    // text
    gui.forEachElement<EC::ViewBox, EC::Text>([&](Element e){
        auto* view = gui.getComponent<EC::ViewBox>(e);
        Box entityBox = view->absolute;
        auto* textComponent = gui.getComponent<EC::Text>(e);
        Vec2 pos;
        switch (textComponent->formatSettings.align.vertical) {
        case VertAlignment::Bottom:
            pos.y = entityBox.min.y;
            break;
        case VertAlignment::Middle:
            pos.y = entityBox.min.y + entityBox.size.y / 2.0f;
            break;
        case VertAlignment::Top:
            pos.y = entityBox.min.y + entityBox.size.y;
            break;
        }
        switch (textComponent->formatSettings.align.horizontal) {
        case HoriAlignment::Left:
            pos.x = entityBox.min.x;
            break;
        case HoriAlignment::Center:
            pos.x = entityBox.min.x + entityBox.size.x / 2.0f;
            break;
        case HoriAlignment::Right:
            pos.x = entityBox.min.x + entityBox.size.x;
            break;
        }
        
        renderer.renderText(textComponent->text, pos, textComponent->formatSettings, textComponent->renderSettings, view->level);
    });

    
    
}

}

#endif