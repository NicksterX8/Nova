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
    gui.addComponent(terminal, EC::StackConstraint{.vertical = true});
    gui.addComponent(terminal, EC::SizeConstraint{
        .minSize = {maxLogWidth, 0}
    });
    gui.addComponent(terminal, EC::Background{terminalBackgroundColor});

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
            strcpy(slotText, str.c_str());
        } else {
            strcpy(slotText, "");
        }
        textEc->text = slotText;
    }
}

}