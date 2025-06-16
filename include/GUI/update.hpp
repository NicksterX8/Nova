#ifndef GUI_UPDATE_INCLUDED
#define GUI_UPDATE_INCLUDED

#include "ecs-gui.hpp"
#include "PlayerControls.hpp"
#include "elements.hpp"

#include "ECS/system.hpp"

#include "GUI/Gui.hpp"

namespace GUI {

#define ADDC(element, componentName, value)

inline Element simpleBox(GuiManager& gui, Element parent, 
    Box box, SDL_Color background, SDL_Color borderColor, float borderSize) {
    Element e = gui.newElement(ElementTypes::Normal, parent);
    gui.addComponent(e, EC::ViewBox{.box = {box.min, box.size}});
    gui.addComponent(e, EC::Background{background});
    gui.addComponent(e, EC::Border{borderColor, Vec2(0), Vec2(borderSize)});
    
    return e;  
}

inline Element textBox(GuiManager& gui, Element parent, Box box, const char* textSource) {
    auto e = simpleBox(gui, parent, box, {255,255,255,255}, {0,0,0,255}, 5);
    gui.addComponent(e, EC::Text{
        .text = textSource,
        .renderSettings = {
            .color = {0,0,0,255}
        }
    });
    return e;
}

inline Element makeHolder(GuiManager& gui, Element element) {
    auto parent = gui.newElement(ElementTypes::Normal);
    gui.adopt(parent, element);
    EC::ViewBox* childViewbox = gui.getComponent<EC::ViewBox>(element);


    return parent;
}

inline Element button(GuiManager& gui, Box box, SDL_Color backgroundColor, const char* text, SDL_Color textColor, GuiAction onClick) {
    auto e = gui.newElement(ElementTypes::Normal);
    gui.addComponent(e, EC::ViewBox{.box = {box.min, box.size}});
    gui.addComponent(e, EC::Background{backgroundColor});
    gui.addComponent(e, EC::Text{
        .text = text,
        .formatSettings = TextFormattingSettings{
            .align = TextAlignment::MiddleCenter},
        .renderSettings = TextRenderingSettings{
            .color = textColor, .scale = Vec2(1)}
    });
    gui.addComponent(e, EC::Border{
        .color = {60, 60, 60, 255},
        .strokeOut = Vec2(2.0f)
    });
    gui.addComponent(e, EC::Button{
        .onClick = onClick
    });
    return e;
}

inline Element buildConsole(GuiManager& gui) {
    float scale = 1;

    SDL_Color terminalBackgroundColor = {30, 30, 30, 205};
    SDL_Color logBackgroundColor = {90, 90, 90, 150};

    float maxLogWidth = 300 * scale;

    // make papa console
    auto console = gui.newElement(ElementTypes::Normal, gui.screen);
    gui.addName(console, "console");
    gui.addComponent(console, EC::ViewBox{.box = {{0,0}, {INFINITY, INFINITY}}, .visible = false});
    gui.addComponent(console, EC::StackConstraint{.vertical = true});

    // make terminal
    auto terminal = gui.newElement(ElementTypes::Normal, console);
    gui.addName(terminal, "console-terminal");
    gui.addComponent(terminal, EC::SizeConstraint{
        .minSize = {maxLogWidth, 0}
    });
    gui.addComponent(terminal, EC::ViewBox{
        .box = {{0,0}, {maxLogWidth, 0}}
    });
    gui.addComponent(terminal, EC::Background{terminalBackgroundColor});
    // gui.addComponent(terminal, EC::Border{
    //     .color = terminalBackgroundColor, .strokeOut = Vec2(5)
    // });


    // make log
    auto log = gui.newElement(ElementTypes::Normal, console);
    gui.addName(log, "console-log");
    gui.addComponent(log, EC::ViewBox{
        .box = {{0,0},{300,0}}
    });
    gui.addComponent(log, EC::Background{logBackgroundColor});
    gui.addComponent(log, EC::SizeConstraint{
        .maxSize = {maxLogWidth, INFINITY},
        .relativeSize = {INFINITY, 1.0}
    });

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
    
    gui.addName(bar, "hotbar");
    gui.addComponent(bar, EC::ViewBox{*rectAsBox(&barRect)});
    gui.addComponent(bar, EC::SizeConstraint{.maxSize = {800 * scale, INFINITY}});
    gui.addComponent(bar, EC::AlignmentConstraint{.alignment = TextAlignment::BottomCenter});
    gui.addComponent(bar, EC::Background({backgroundColor}));
    gui.addComponent(bar, EC::Border({borderColor, Vec2(0), Vec2(borderSize)}));

    // slots
    float innerMargin = 2 * scale;

    float offset = borderSize;
    
    EC::Name name;
    for (int s = 0; s < numSlots; s++) {
        auto slot = gui.newElement(ElementTypes::Normal, bar);
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
        gui.addComponent(slot, EC::Text{
            .formatSettings = TextFormattingSettings{
                .align = TextAlignment::BottomLeft},
            .renderSettings = TextRenderingSettings{
                .color = {255,255,255,255}, .scale = Vec2(textScale)}
        });
        gui.addComponent(slot, EC::Button{selectHotbarSlot});
    }

    return bar;
}

inline void initGui(GuiManager& gui) {
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
    auto console = buildConsole(gui);
    auto testTextBox = textBox(gui, gui.screen, Box{Vec2(100), Vec2(200)}, "This is some really awesome text and stuff");
    //auto halfBox = boxElement(gui, {Vec2(0.0f), Vec2(50.0f)}, {150, 0, 0, 255});
    //gui.addComponent(halfBox, EC::SizeConstraint{.relativeSize = Vec2(0.5, 0.2)});
    auto t2 = textBox(gui, gui.screen, Box{Vec2(0, 400), Vec2(350)}, "This is some really awesome text and stuff fjdsfi i i i i i i i i i i i i i i i i i i I hate you");
    auto t3 = textBox(gui, gui.screen, Box{Vec2(1500), Vec2(800)}, "This is some really awesome text and stuff");
    gui.getComponent<EC::Text>(t2)->formatSettings.align = TextAlignment::MiddleCenter;
    gui.getComponent<EC::Text>(t3)->formatSettings.align = TextAlignment::BottomRight;

    auto b = button(gui, 
        Box{{700, 100}, {100, 100}},
        {0, 180, 30, 255},
        "Execute them.", 
        {255,255,255,255}, 
        makeTheSkyBlue);
    gui.adopt(gui.screen, b);
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
    
    gui.forEachEntity([&](auto signature){
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

    
    
}


#endif