#include "GUI/Gui.hpp"
#include "rendering/gui.hpp"

/*

void Draw::itemStack(SDL_Renderer* renderer, float scale, ItemStack item, SDL_Rect* destination) {
    // icon
    SDL_RenderCopy(renderer, ItemData[item.item].icon, NULL, destination);
    // quantity count
    // dont draw item count over items that can only ever have one count,
    // its pointless
    if (ItemData[item.item].stackSize != 1 && item.quantity != ItemQuantityInfinity) {
        FC_DrawScale(FreeSans, renderer, destination->x + 3, destination->y - 4, FC_MakeScale(scale/2.0f,scale/2.0f),
        "%d", item.quantity);
    }
}

*/

SDL_FRect GUI::Hotbar::draw(GuiRenderer& renderer, SDL_Rect viewport, const Player* player, const ItemManager& itemManager) {
    float scale = renderer.options.scale;

    slots.clear();

    unsigned int numSlots = player->numHotbarSlots;
    auto* inventory = player->inventory();
    if (!inventory || numSlots == 0) {
        rect = {0, 0, 0, 0};
        return rect;
    }

    const float opacity = 0.5;
    Uint8 alpha = opacity * 255;
    float width = viewport.w;
    float height = viewport.h;

    const float maxSlotSize = 60 * scale;
    const float borderSize = 3 * scale;
    float slotSize = maxSlotSize;
    if (slotSize * numSlots > (width - borderSize * 2)) {
        slotSize = (width - borderSize * 2) / numSlots;
    }
    float hotbarWidth = slotSize * numSlots;
    float borderHeight = slotSize + borderSize*2;
    
    float maxWidth = (slotSize * numSlots) + borderSize*2;
    float borderWidth = width;
    if (borderWidth > maxWidth) {
        borderWidth = maxWidth;
    }

    float horizontalMargin = (width - borderWidth) / 2;
    SDL_FRect border = {
        horizontalMargin + viewport.x,
        0,
        borderWidth,
        borderHeight 
    };

    this->rect = border;
    
    SDL_FRect inside = {
        border.x + borderSize,
        border.y + borderSize,
        border.w - borderSize * 2,
        border.h - borderSize * 2
    };

    renderer.colorRect({border, {60, 60, 60, alpha}});
    renderer.colorRect({inside, {100, 100, 100, alpha}});

    {
        float hotbarHorizontalMargin = (inside.w - hotbarWidth);
        float hotbarVerticalMargin = (inside.h - slotSize);
        SDL_FRect hotbarSlot = {
            inside.x,
            inside.y + hotbarVerticalMargin / 2,
            slotSize,
            slotSize
        };
        
        for (unsigned int i = 0; i < player->numHotbarSlots; i++) {
            slots.push(hotbarSlot);

            ItemStack item = inventory->get(i);

            // draw slot in hotbarSlot
            renderer.colorRect({hotbarSlot, {60, 60, 60, alpha}});
            renderer.colorRect({hotbarSlot, {30, 30, 30, alpha}});

            float innerMargin = 2 * scale;
            SDL_FRect innerSlot = {
                hotbarSlot.x + innerMargin,
                hotbarSlot.y + innerMargin,
                hotbarSlot.w - innerMargin*2,
                hotbarSlot.h - innerMargin*2
            };

            if (item.item.type) {
                const ItemPrototype* prototype = items::getPrototype(item.item.type, itemManager);
                // icon
                //renderer.textureArraySprite(prototype->inventoryIcon, innerSlot);
                // quantity count
                // dont draw item count over items that can only ever have one count,
                // its pointless
                if (prototype->stackSize != 1 && item.quantity != items::ItemQuantityInfinity) {
                    char text[128];
                    snprintf(text, 128, "%d", item.quantity);
                    glm::vec2 pos = {hotbarSlot.x, hotbarSlot.y};
                    renderer.text->render(text, pos, {TextAlignment::BottomLeft}, TextRenderingSettings(glm::vec2(scale/2.0f)));
                }
            }
            
            // divide margin by 8, not 9 because there are 8 spaces between 9 slots, not 9.
            hotbarSlot.x += slotSize + hotbarHorizontalMargin / (numSlots - 1);
        }

        // draw highlight around selected hotbar slot
        SDL_FRect selectedHotbarSlot = {
            inside.x + player->selectedHotbarStack * (slotSize + hotbarHorizontalMargin / (numSlots - 1)),
            inside.y + hotbarVerticalMargin / 2,
            slotSize,
            slotSize
        };
        renderer.colorRect({selectedHotbarSlot, {0, 0, 0, 255}});
    }

    return rect;
}

void GUI::Gui::drawHeldItemStack(GuiRenderer& renderer, SDL_Rect viewport) {
    SDL_Point mousePosition = SDL::getMousePixelPosition();
    const int size = 60 * renderer.options.scale;
    SDL_Rect destination = {
        mousePosition.x - size/2,
        mousePosition.y - size/2,
        size,
        size
    };
    //Draw::itemStack(ren, scale, *heldItemStack, &destination);
}