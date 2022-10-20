#ifndef GUI_INCLUDED
#define GUI_INCLUDED

#include "../sdl_gl.hpp"
#include <vector>
#include "../Items.hpp"
#include "../constants.hpp"
#include "../rendering/Drawing.hpp"
#include "../Player.hpp"

namespace Draw {
    void itemStack(SDL_Renderer* renderer, float scale, ItemStack item, SDL_Rect* destination);
}

namespace GUI {

/*

class Hotbar {
public:
    SDL_FRect rect; // the rectangle outline of the hotbar, updated every time draw() is called
    std::vector<SDL_FRect> slots; // the rectangle outlines of the hotbar slots, updated every time draw() is called

    SDL_FRect draw(SDL_Renderer* ren, float scale, SDL_Rect viewport, const Player* player) {
        slots.clear();

        unsigned int numSlots = player->numHotbarSlots;
        if (!player->inventory() || numSlots == 0) {
            rect = {0, 0, 0, 0};
            return rect;
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);

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
            height - (borderHeight) + viewport.y,
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

        SDL_SetRenderDrawColor(ren, 60, 60, 60, 255);
        Draw::thickRect(ren, &border, borderSize * scale);
        SDL_SetRenderDrawColor(ren, 100, 100, 100, alpha);
        SDL_RenderFillRectF(ren, &inside);
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
                slots.push_back(hotbarSlot);

                ItemStack item = player->inventory()->get(i);

                // draw slot in hotbarSlot
                SDL_SetRenderDrawColor(ren, 60, 60, 60, alpha);
                SDL_RenderFillRectF(ren, &hotbarSlot);
                SDL_SetRenderDrawColor(ren, 30, 30, 30, alpha);
                Draw::thickRect(ren, &hotbarSlot, 2*scale);
                float innerMargin = 2 * scale;
                SDL_FRect innerSlot = {
                    hotbarSlot.x + innerMargin,
                    hotbarSlot.y + innerMargin,
                    hotbarSlot.w - innerMargin*2,
                    hotbarSlot.h - innerMargin*2
                };

                if (item.item) {
                    // icon
                    SDL_RenderCopyF(ren, ItemData[item.item].icon, NULL, &innerSlot);
                    // quantity count
                    // dont draw item count over items that can only ever have one count,
                    // its pointless
                    if (ItemData[item.item].stackSize != 1 && item.quantity != INFINITE_ITEM_QUANTITY) {
                        FC_DrawScale(FreeSans, ren, hotbarSlot.x + 3, hotbarSlot.y - 4, FC_MakeScale(scale/2.0f,scale/2.0f),
                        "%d", item.quantity);
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
            SDL_SetRenderDrawColor(ren, 0, 255, 255, 155);
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

        return rect;
    }
};
*/

struct Console {
    My::StringBuffer text = My::StringBuffer::FromStr("\0");
    My::Vec<glm::vec4> textColors = My::Vec<glm::vec4>::Empty();

    void pushNewLine() {

    }
};  

struct Gui {
    My::Vec<SDL_FRect> area = My::Vec<SDL_FRect>(0);
    ItemStack* heldItemStack = nullptr;
    Console console = {};

    Gui() {
        console.textColors.push({0, 0, 0, 1});
    }

    void draw(SDL_Renderer* ren, float scale, SDL_Rect viewport, const Player* player) {
        area.size = 0;
        //SDL_FRect hotbarArea = hotbar.draw(ren, scale, viewport, player);
        heldItemStack = player->heldItemStack;
        if (heldItemStack && heldItemStack->item)
            drawHeldItemStack(ren, scale, viewport);
        //area.push_back(hotbarArea);
    }

    void drawHeldItemStack(SDL_Renderer* ren, float scale, SDL_Rect viewport) {
        SDL_Point mousePosition = SDL::getMousePixelPosition();
        int size = 60 * scale;
        SDL_Rect destination = {
            mousePosition.x - size/2,
            mousePosition.y - size/2,
            size,
            size
        };
        Draw::itemStack(ren, scale, *heldItemStack, &destination);
    }

    bool pointInArea(SDL_Point point) const {
        for (size_t i = 0; i < area.size; i++) {
            if (point.x > area[i].x && point.x < area[i].x + area[i].w &&
                point.y > area[i].y && point.y < area[i].y + area[i].h) {
                return true;
            }
        }
        return false;
    }
};

}

using GUI::Gui;

#endif