#include "GUI/update.hpp"
#include "Game.hpp"

namespace GUI {

void updateHotbar(Game* g, Element hotbarElement) {
    auto& manager = g->gui->manager;
    auto* inventory = g->state->player.inventory();
    if (!inventory) {
        manager.hideElement(hotbarElement);
        return;
    } else {
        manager.unhideElement(hotbarElement);
    }
}

void updateHotbarSlot(Game* game, Element slot) {
    auto* playerInventory = game->state->player.inventory();
    if (!playerInventory) return;

    auto& manager = game->gui->manager;
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