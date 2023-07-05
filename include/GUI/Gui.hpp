#ifndef GUI_INCLUDED
#define GUI_INCLUDED

#include "My/Vec.hpp"
#include "sdl_gl.hpp"
#include "items/items.hpp"
#include "constants.hpp"
#include "Player.hpp"
#include "commands.hpp"

struct GuiRenderer;

namespace GUI {

class Hotbar {
public:
    SDL_FRect rect; // the rectangle outline of the hotbar, updated every time draw() is called
    My::Vec<SDL_FRect> slots; // the rectangle outlines of the hotbar slots, updated every time draw() is called

    Hotbar() {
        rect = {0, 0, 0, 0};
        slots = My::Vec<SDL_FRect>::Empty();
    }

    SDL_FRect draw(GuiRenderer& renderer, SDL_Rect viewport, const Player* player, const ItemManager& itemManager);

    void destroy() {
        slots.destroy();
    }
};

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
                    if (ItemData[item.item].stackSize != 1 && item.quantity != ItemQuantityInfinity) {
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
    enum class MessageType {
        Default,
        Command,
        CommandResult
    };

    My::StringBuffer log = My::StringBuffer::FromStr("\0");
    My::Vec<MessageType> logMessageTypes = My::Vec<MessageType>::Empty();
    My::Vec<SDL_Color> logTextColors = My::Vec<SDL_Color>::Empty();

    std::string activeMessage;
    int logHistoryIndex = -1; // going through the log, to retype and old message, this index is for seeing what message its on. -1 means not using the log history currently
    

    constexpr static SDL_Color activeTextColor = {255, 255, 255, 255};
    constexpr static SDL_Color defaultTextColor = {200, 200, 200, 255};
    constexpr static SDL_Color commandTextColor = {200, 200, 100, 255};
    constexpr static SDL_Color commandResultTextColor = {100, 100, 50, 255};

    bool messageIsActive() const {
        return !activeMessage.empty();
    }

    void pushActiveMessage(MessageType type) {
        if (activeMessage.empty()) return;
        newMessage(activeMessage.c_str(), type);
        activeMessage.clear();
    }

    void newMessage(const char* message, MessageType type) {
        log.push(message);
        logMessageTypes.push(type);

        SDL_Color color = defaultTextColor;
        switch (type) {
        case MessageType::Command:
            color = commandTextColor;
            break;
        case MessageType::CommandResult:
            color = commandResultTextColor;
            break;
        default:
            break;
        }

        logTextColors.push(color);
    }

    // will clamp indices below or above the limit to an empty message or oldest message, respectively
    // TODO: fix name
    void moveLoggedMessage(int offset) {
        if (offset == 0) return;
        int dir = (offset > 0) ? 1 : -1;
        int historyIndex = logHistoryIndex + offset;

        if (historyIndex < 0) {
            activeMessage = "";
            logHistoryIndex = -1;
            return;
        }

        int i = -1;
        const char* msg = nullptr;
        log.forEachStrReverse([&](const char* str){
            int wouldBeIndex = i+1;
            if (wouldBeIndex > historyIndex) {
                return true; // means: break
            }
            // skip over message results - they don't count
            int wouldBeReverseIndex = logMessageTypes.size - wouldBeIndex - 1;
            if (logMessageTypes[wouldBeReverseIndex] == MessageType::CommandResult && wouldBeIndex + (dir == -1) >= historyIndex) {
                historyIndex += dir;
            } else {
                msg = str;
            }
            i = wouldBeIndex;
            return false; // means: dont break
        });

        if (!msg) LogError("Failed to select logged message!");
        activeMessage = msg ? std::string(msg) : "";
        logHistoryIndex = i; // we set the new index to i, not historyIndex because we might not actually end up on that one,
        // if the index is too great (see method desc)
    }

    CommandInput enterText(SDL_Keycode keycode, ArrayRef<Command> possibleCommands) {
        CommandInput command = {};
        switch (keycode) {
        case SDLK_RETURN2:
        case SDLK_RETURN: {
            logHistoryIndex = -1;

            command = processMessage(activeMessage, possibleCommands);
            if (command) {
                pushActiveMessage(MessageType::Command);
            } else {
                // not command
                pushActiveMessage(MessageType::Default);
            }
            break;
        }
        case SDLK_DELETE:
        case SDLK_BACKSPACE:
            if (!activeMessage.empty()) {
                activeMessage.pop_back();
            }
            break;
        case SDLK_TAB:
            activeMessage.push_back('\t');
            break;
        case SDLK_UP:
            moveLoggedMessage(1);
            break;
        case SDLK_DOWN:
            moveLoggedMessage(-1);
            break;
        } // end keycode switch

        return command;
    }

    void destroy() {
        log.destroy();
        logMessageTypes.destroy();
        logTextColors.destroy();
    }
};


struct Gui {
    My::Vec<SDL_FRect> area = My::Vec<SDL_FRect>(0);
    Hotbar hotbar;
    ItemStack* heldItemStack = nullptr;
    Console console = {};
    bool consoleOpen = false;

    Gui() {}

    void draw(GuiRenderer& renderer, float scale, SDL_Rect viewport, const Player* player, const ItemManager& itemManager) {
        area.size = 0;
        SDL_FRect hotbarArea = hotbar.draw(renderer, viewport, player, itemManager);
        heldItemStack = player->heldItemStack;
        if (heldItemStack && !heldItemStack->empty())
            drawHeldItemStack(renderer, viewport);
        area.push(hotbarArea);
    }

    void drawHeldItemStack(GuiRenderer& renderer, SDL_Rect viewport);

    bool pointInArea(SDL_Point point) const {
        for (int i = 0; i < area.size; i++) {
            if (point.x > area[i].x && point.x < area[i].x + area[i].w &&
                point.y > area[i].y && point.y < area[i].y + area[i].h) {
                return true;
            }
        }
        return false;
    }

    void destroy() {
        area.destroy();
        hotbar.destroy();
        console.destroy();
    }
};

}

using GUI::Gui;

#endif