#ifndef GUI_INCLUDED
#define GUI_INCLUDED

#include <vector>
#include <SDL2/SDL.h>
#include "constants.hpp"
#include "rendering.hpp"
#include "Player.hpp"

namespace Draw {
    void thickRect(SDL_Renderer* renderer, const SDL_FRect* rect, int thickness);
}

class Hotbar {
public:
    SDL_FRect rect;
    SDL_FRect slots[NUM_HOTBAR_SLOTS];

    SDL_FRect area() const {
        return rect;
    }

    SDL_FRect draw(SDL_Renderer* ren, float scale, SDL_Rect viewport, const Player* player) {
        //SDL_RenderSetScale(ren, 1, 1);
        float width = viewport.w;
        float height = viewport.h;
        float borderHeight = 80 * scale;
        float borderSize = 3 * scale;
        float maxWidth = (borderHeight - borderSize*2) * NUM_HOTBAR_SLOTS + borderSize*2;
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
        rect = border;
        
        SDL_FRect inside = {
            border.x + borderSize,
            border.y + borderSize,
            border.w - borderSize * 2,
            border.h - borderSize * 2
        };
        SDL_SetRenderDrawColor(ren, 60, 60, 60, 255);
        SDL_RenderFillRectF(ren, &border);
        SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
        SDL_RenderFillRectF(ren, &inside);

        {
            float hotbarSlotWidth = inside.w / NUM_HOTBAR_SLOTS;
            float hotbarSlotHeight = inside.h;
            // enforce square hotbar slots by getting minimum dimension
            float slotSize = hotbarSlotWidth;
            if (hotbarSlotHeight < hotbarSlotWidth) {
                slotSize = hotbarSlotHeight;
            }
            float hotbarWidth = slotSize * NUM_HOTBAR_SLOTS;
            float hotbarHorizontalMargin = (inside.w - hotbarWidth);
            float hotbarVerticalMargin = (inside.h - slotSize);
            SDL_FRect hotbarSlot = {
                inside.x,
                inside.y + hotbarVerticalMargin / 2,
                slotSize,
                slotSize
            };
            
            for (int i = 0; i < NUM_HOTBAR_SLOTS; i++) {
                slots[i] = hotbarSlot;

                ItemStack item = player->inventory.items[i];

                // draw slot in hotbarSlot
                SDL_SetRenderDrawColor(ren, 60, 60, 60, 255);
                SDL_RenderFillRectF(ren, &hotbarSlot);
                SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
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
                    FC_DrawScale(FreeSans, ren, hotbarSlot.x + 3, hotbarSlot.y - 4, FC_MakeScale(scale/2.0f,scale/2.0f),
                    "%d", item.quantity);
                }
                
                // divide margin by 8, not 9 because there are 8 spaces between 9 slots, not 9.
                hotbarSlot.x += slotSize + hotbarHorizontalMargin / (NUM_HOTBAR_SLOTS - 1);
            }

            // draw highlight around selected hotbar slot
            SDL_FRect selectedHotbarSlot = {
                inside.x + player->selectedHotbarStack * (slotSize + hotbarHorizontalMargin / (NUM_HOTBAR_SLOTS - 1)),
                inside.y + hotbarVerticalMargin / 2,
                slotSize,
                slotSize
            };
            SDL_SetRenderDrawColor(ren, 0, 255, 255, 155);
            Draw::thickRect(ren, &selectedHotbarSlot, 2);
        }

        //SDL_RenderSetScale(ren, 1, 1);

        return area();
    }
};

struct GUI {
    Hotbar hotbar;
    std::vector<SDL_FRect> area;

    void draw(SDL_Renderer* ren, float scale, SDL_Rect viewport, const Player* player) {
        area.clear();
        SDL_FRect hotbarArea = hotbar.draw(ren, scale, viewport, player);
        area.push_back(hotbarArea);
    }

    bool pointInArea(SDL_Point point) const {
        for (size_t i = 0; i < area.size(); i++) {
            if (point.x > area[i].x && point.x < area[i].x + area[i].w &&
                point.y > area[i].y && point.y < area[i].y + area[i].h) {
                return true;
            }
        }
        return false;
    }
};

#endif