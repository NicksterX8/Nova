#include "GUI/update.hpp"
#include "Game.hpp"

namespace GUI {

Element buildConsole(GuiManager& gui) {
    float scale = 1;

    SDL_Color terminalBackgroundColor = {0, 0, 0, 255};
    SDL_Color logBackgroundColor = {20, 20, 20, 255};

    float maxLogWidth = ConsoleLogWidth * scale;

    // make papa console
    auto console = gui.newElement(ElementTypes::Normal, gui.screen);
    gui.addName(console, "console");
    addView(gui, console, {{0,0}, {INFINITY, INFINITY}}, GUI::RenderLevel::Console, false);
    gui.addComponent(console, EC::StackConstraint{.vertical = true});

    // make terminal
    auto terminal = gui.newElement(ElementTypes::Normal, console);
    gui.addName(terminal, "console-terminal");
    addView(gui, terminal, {{0,0},{maxLogWidth,0}});
    gui.addComponent(terminal, EC::SizeConstraint{
        .minSize = {maxLogWidth, 0}
    });
    gui.addComponent(terminal, EC::Background{terminalBackgroundColor});

    auto line = gui.newElement(ElementTypes::Normal, console);
    addView(gui, line, {{0,0},{maxLogWidth,5}});
    gui.addComponent(line, EC::Background{{255, 0, 0, 255}});
    hideIf(gui, line, [terminal](Game* game, GuiManager& manager, Element line){
        return manager.entityHas<EC::Hidden>(terminal);
    });

    // make log
    auto log = gui.newElement(ElementTypes::Normal, console);
    gui.addName(log, "console-log");
    addView(gui, log, {{0,0}, {ConsoleLogWidth, 0}});
    gui.addComponent(log, EC::Background{logBackgroundColor});
    gui.addComponent(log, EC::SizeConstraint{
        .maxSize = {maxLogWidth, INFINITY},
        .relativeSize = {INFINITY, 1.0}
    });

    return console;
}

Element buildHotbar(GuiManager& gui) {
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
        .update = {(GuiAction)[](Game* game, GuiManager& manager, Element hotbarElement){
            auto* inventory = game->state->player.inventory();
            if (!inventory) {
                manager.hideElement(hotbarElement);
                return;
            } else {
                manager.unhideElement(hotbarElement);
            }
        }}
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
        gui.addComponent(slot, EC::Button{.onClick = {selectHotbarSlot}});
        gui.addComponent(slot, EC::Update{
            .update = {updateHotbarSlot}
        });
    }

    return bar;
}

void updateHotbar(Game* g, GuiManager& manager, Element hotbarElement) {
    auto* inventory = g->state->player.inventory();
    if (!inventory) {
        manager.hideElement(hotbarElement);
        return;
    } else {
        manager.unhideElement(hotbarElement);
    }
}

void updateHotbarSlot(Game* game, GuiManager& manager, Element slot) {
    auto* playerInventory = game->state->player.inventory();
    if (!playerInventory) return;

    auto* slotComponent = manager.getComponent<EC::HotbarSlot>(slot);
    ItemStack stack = playerInventory->get(slotComponent->slot);

    auto* textureEc = manager.getComponent<EC::SimpleTexture>(slot);
    auto* textEc = manager.getComponent<EC::Text>(slot);

    auto* displayIec = game->state->itemManager.getComponent<ITC::Display>(stack.item);
    ItemQuantity stackSize = items::getStackSize(stack.item, game->state->itemManager);

    if (textureEc) {
        textureEc->texture = displayIec ? displayIec->inventoryIcon : TextureIDs::Null;
    }

    if (textEc) {
        char* slotText = slotComponent->text;
        if (stack.quantity != 0 && stackSize != 1) {
            auto str = string_format("%d", stack.quantity);
            strlcpy(slotText, str.c_str(), 16);
        } else {
            strcpy(slotText, "");
        }
        textEc->text = slotText;
    }
}

}