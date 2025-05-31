#include "GUI/Gui.hpp"
#include "rendering/gui.hpp"
#include "rendering/drawing.hpp"

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

SDL_FRect GUI::Hotbar::draw(GuiRenderer& renderer, const Player* player, const ItemManager& itemManager) {
    #define HOTBAR_FONT "Gui"
    auto font = Fonts->get(HOTBAR_FONT);
    
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
    float width = renderer.options.size.x;
    float height = renderer.options.size.y;

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
        horizontalMargin,
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

    renderer.colorRect(border, {60, 60, 60, alpha});
    renderer.colorRect(inside, {100, 100, 100, alpha});

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
            renderer.colorRect(hotbarSlot, {60, 60, 60, alpha});
            renderer.colorRect(hotbarSlot, {30, 30, 30, alpha});

            float innerMargin = 2 * scale;
            SDL_FRect innerSlot = {
                hotbarSlot.x + innerMargin,
                hotbarSlot.y + innerMargin,
                hotbarSlot.w - innerMargin*2,
                hotbarSlot.h - innerMargin*2
            };

            if (!item.empty()) {
                const ItemPrototype* prototype = items::getPrototype(item.item.type, itemManager);
                const ITC::Display* displayComponent = items::getComponent<ITC::Display>(item.item, itemManager);
                if (displayComponent && prototype) {
                    // icon
                    renderer.sprite(displayComponent->inventoryIcon, innerSlot);
                    // quantity count
                    // dont draw item count over items that can only ever have one count,
                    // its pointless
                    if (prototype->get<ITC::StackSize>()->quantity != 1 && item.quantity != items::ItemQuantityInfinity) {
                        char text[128];
                        snprintf(text, 128, "%d", item.quantity);
                        float textScale = scale/2.0f;
                        glm::vec2 pos = Vec2{hotbarSlot.x, hotbarSlot.y + font->descender() * textScale} + renderer.pixels({2.5f, 5.0f});
                        renderer.text->render(text, pos, {TextAlignment::BottomLeft},
                            TextRenderingSettings(font, glm::vec2(textScale))
                            );
                    }
                }
            }
            
            // divide margin by 8, not 9 because there are 8 spaces between 9 slots, not 9.
            hotbarSlot.x += slotSize + hotbarHorizontalMargin / (numSlots - 1);
        }

        // draw highlight around selected hotbar slot
        int selectedHotbarSlot = player->selectedHotbarStack;
        if (selectedHotbarSlot >= 0 && selectedHotbarSlot < (int)player->numHotbarSlots) {
            SDL_FRect selectedHotbarSlotRect = {
                inside.x + selectedHotbarSlot * (slotSize + hotbarHorizontalMargin / (numSlots - 1)),
                inside.y + hotbarVerticalMargin / 2,
                slotSize,
                slotSize
            };
            renderer.rectOutline(selectedHotbarSlotRect, {120, 200, 250, 200}, renderer.pixels(2.0f), 0.0f);
        }
    }

    return rect;
}

void GUI::Gui::drawHeldItemStack(GuiRenderer& renderer, const ItemManager& itemManager, const ItemStack& itemStack, glm::vec2 pos) {
    if (itemStack.empty()) return;
    const float size = renderer.pixels(60);
    Vec2 bottomLeft = {
        pos.x - size/2,
        pos.y - size/2
    };
    FRect destination = {
        bottomLeft.x,
        bottomLeft.y,
        size,
        size
    };
    Draw::drawItemStack(renderer, itemManager, itemStack, destination);
    float textScale = renderer.options.scale/2.0f;
    pos = Vec2{bottomLeft.x, bottomLeft.y + Fonts->get("Gui")->descender() * textScale} + renderer.pixels({2.5f, 5.0f});
    renderer.text->render(string_format("%d", itemStack.quantity).c_str(), pos,
        TextFormattingSettings(TextAlignment::BottomLeft), TextRenderingSettings(Fonts->get("Gui"), {0,0,0,255}, Vec2(textScale)));
}