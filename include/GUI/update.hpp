#ifndef GUI_UPDATE_INCLUDED
#define GUI_UPDATE_INCLUDED

#include "ecs-gui.hpp"
#include "PlayerControls.hpp"
#include "elements.hpp"

#include "ECS/system.hpp"

namespace GUI {

#define ADDC(element, componentName, value)

inline Element simpleBox(GuiManager& gui, Element parent, 
    Box box, SDL_Color background, SDL_Color borderColor, float borderSize) {
    Element e = gui.newElement(ElementTypes::Normal, parent);
    addView(gui, e, box);
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

inline Element button(GuiManager& gui, Box box, SDL_Color backgroundColor, const char* text, SDL_Color textColor, GuiAction onClick, GUI::RenderLevel level) {
    auto e = gui.newElement(ElementTypes::Normal);
    addView(gui, e, box, level);
    gui.addComponent(e, EC::Background{backgroundColor});
    gui.addComponent(e, EC::Text{
        .text = text,
        .formatSettings = TextFormattingSettings{
            .align = TextAlignment::MiddleCenter},
        .renderSettings = TextRenderingSettings{
            .color = textColor, .scale = 1.0f}
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

constexpr float ConsoleLogWidth = 400.0f;

Element buildConsole(GuiManager& gui);

void updateHotbar(Game* game, GuiManager&, Element hotbarElement);
void updateHotbarSlot(Game* game, GuiManager&, Element slotElement);

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
    addView(gui, bar, *rectAsBox(&barRect), GUI::RenderLevel::Hotbar);
    gui.addComponent(bar, EC::SizeConstraint{.maxSize = {800 * scale, INFINITY}});
    gui.addComponent(bar, EC::AlignmentConstraint{.alignment = TextAlignment::BottomCenter});
    gui.addComponent(bar, EC::Background({backgroundColor}));
    gui.addComponent(bar, EC::Border({borderColor, Vec2(0), Vec2(borderSize)}));
    gui.addComponent(bar, EC::Update{
        .update = updateHotbar
    });

    // slots
    float innerMargin = 2 * scale;

    float offset = borderSize;
    
    EC::Name name;
    for (int s = 0; s < numSlots; s++) {
        auto slot = gui.newElement(ElementTypes::Normal, bar);
        snprintf(name.name, 64, "hotbar-slot-%d", s);
        gui.addComponent(slot, name);
        gui.addComponent(slot, EC::HotbarSlot{.slot = s});

        Box hotbarSlot = {
            {offset, borderSize},
            Vec2{slotSize}
        };

        Box innerSlot = {
            Vec2(innerMargin),
            hotbarSlot.size - Vec2(innerMargin*2) 
        };

        offset += slotSize;

        addView(gui, slot, hotbarSlot);
        gui.addComponent(slot, EC::SimpleTexture{0, innerSlot});
        gui.addComponent(slot, EC::Background{slotBackgroundColor});
        gui.addComponent(slot, EC::Hover{true, slotHoverColor, 0});
        float textScale = scale/2.0f;
        gui.addComponent(slot, EC::Text{
            .formatSettings = TextFormattingSettings{
                .align = TextAlignment::BottomLeft},
            .renderSettings = TextRenderingSettings{
                .color = {255,255,255,255}, .scale = textScale}
        });
        gui.addComponent(slot, EC::Button{selectHotbarSlot});
        gui.addComponent(slot, EC::Update{
            .update = updateHotbarSlot
        });
    }

    return bar;
}

inline void initGui(GuiManager& gui) {

    auto bar = buildHotbar(gui);
    auto console = buildConsole(gui);
    // auto b = button(gui, 
    //     Box{{700, 100}, {100, 100}},
    //     {0, 180, 30, 255},
    //     "Execute them.", 
    //     {255,255,255,255}, 
    //     makeTheSkyBlue);
    //auto testTextBox = textBox(gui, gui.screen, Box{Vec2(100), Vec2(200)}, "This is some really awesome text and stuff");
    //auto halfBox = boxElement(gui, {Vec2(0.0f), Vec2(50.0f)}, {150, 0, 0, 255});
    //gui.addComponent(halfBox, EC::SizeConstraint{.relativeSize = Vec2(0.5, 0.2)});
    //auto t2 = textBox(gui, gui.screen, Box{Vec2(0, 400), Vec2(350)}, "This is some really awesome text and stuff fjdsfi i i i i i i i i i i i i i i i i i i I hate you");
    //auto t3 = textBox(gui, gui.screen, Box{Vec2(1000), Vec2(800)}, "This is some really awesome text and stuff");
    //gui.getComponent<EC::Text>(t2)->formatSettings.align = TextAlignment::MiddleCenter;
    //gui.getComponent<EC::Text>(t3)->formatSettings.align = TextAlignment::BottomRight;

    auto b = button(gui, 
        Box{{700, 50}, {100, 100}},
        {0, 180, 30, 255},
        "Execute them.", 
        {255,255,255,255}, 
        makeTheSkyBlue,
        GUI::RenderLevel::Lowest);
    gui.adopt(gui.screen, b);
}

inline void bufferAction(GuiAction guiAction, GUI::Element e, std::vector<GameAction>* gameActions) {
    gameActions->push_back(guiActionToGameAction(guiAction, e));
}

inline std::vector<GameAction> update(GuiManager& gui, const PlayerControls& playerControls) {
    Vec2 mousePos = playerControls.mousePixelPos();
    bool mouseLeftDown = playerControls.mouse.leftButtonDown();
    bool mouseClicked = playerControls.mouseClicked;

    Global.mouseClickedOnGui = false;

    std::vector<GameAction> actions;
    
    gui.forEachEntity([&](auto signature){
        return signature[EC::DisplayBox::ID] && signature[EC::Hover::ID];
    }, [&](Element e){
        auto* displayBox = gui.getComponent<EC::DisplayBox>(e);
        auto* hover = gui.getComponent<EC::Hover>(e);
        Box box = displayBox->box;

        bool mouseOnButton = pointInRect(mousePos, box.rect());
        if (mouseOnButton) {
            // do something when mouse goes over element?
        }
    });

    return actions;
}

    
    
}


#endif