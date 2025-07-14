#include "actions.hpp"
#include "Game.hpp"
#include "GUI/components.hpp"

GameAction guiActionToGameAction(GuiAction function, GUI::Element e) {
    return {
        [e, function](Game* g){ function(g, g->gui.manager, e); },
        0
    };
}

namespace GUI {
// gui actions
void selectHotbarSlot(Game* g, GuiManager& manager, Element e) {
    int slot = manager.getComponent<EC::HotbarSlot>(e)->slot;
    auto* playerInventory = g->state->player.inventory();
    if (!playerInventory) {
        return;
    }
    ItemStack* stack = &playerInventory->items[slot];
    if (g->state->player.heldItemStack) {
        // set held stack down in slot only if actually holding stack and if slot is empty
        if (stack->empty()) {
            *stack = *g->state->player.heldItemStack.get();
            *g->state->player.heldItemStack.get() = ItemStack::None();
            g->state->player.heldItemStack = ItemHold();
            g->state->player.selectedHotbarSlot = -1;
        } else {
            bool combined = items::combineStacks(stack, g->state->player.heldItemStack.get(), g->state->itemManager);
            if (combined) {
                
            } else {
                // stop holding item but dont move anything
            }
        }
    } else {
        g->state->player.pickupItem(*stack);
        g->state->player.selectHotbarSlot(slot);
        *stack = ItemStack::None();
        g->playerControls->justGrabbedItem = true;
    }
}

void makeTheSkyBlue(Game* g, GuiManager&, Element e) {
    LogInfo("Sky is blue. Element id: %d", e.id);
}

}