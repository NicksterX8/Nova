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

    SDL_FRect draw(GuiRenderer& renderer, const Player* player, const ItemManager& itemManager);

    void destroy() {
        slots.destroy();
    }
};

struct Console {
    enum class MessageType {
        Default,
        Command,
        CommandResult,
        Error,
        NumTypes
    };

    constexpr static SDL_Color typeColors[(int)MessageType::NumTypes] = {
        {230, 230, 230, 255},
        {220, 220, 60, 255},
        {150, 150, 50, 255},
        {255, 0, 0, 255}
    };

    My::StringBuffer log = My::StringBuffer::FromStr("\0");
    My::Vec<MessageType> logMessageTypes = My::Vec<MessageType>::Empty();
    My::Vec<SDL_Color> logTextColors = My::Vec<SDL_Color>::Empty();

    std::string activeMessage;
    int logHistoryIndex = -1; // going through the log, to retype and old message, this index is for seeing what message its on. -1 means not using the log history currently
    int selectedCharIndex = 0;

    #define CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION secondsToTicks(8)
    Tick tLastMessageSent = NullTick;

    bool showLog = false;
    bool promptOpen = false;

    constexpr static SDL_Color activeTextColor = {255, 255, 255, 255};

    bool messageIsActive() const {
        return !activeMessage.empty();
    }

    void pushActiveMessage(MessageType type) {
        if (activeMessage.empty()) return;
        newMessage(activeMessage.c_str(), type);
        activeMessage.clear();

        selectedCharIndex = 0;
        logHistoryIndex = -1;
    }

    void newMessage(const char* message, MessageType type) {
        log.push(message);
        logMessageTypes.push(type);

        SDL_Color color = typeColors[(int)type];

        logTextColors.push(color);
        tLastMessageSent = getTick();
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
            if (wouldBeReverseIndex < 0) return true;
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
        selectedCharIndex = activeMessage.size();
    }

    void insertChar(char c) {
        int index = MAX(MIN(selectedCharIndex, activeMessage.size()), 0);
        activeMessage.insert(index, 1, c);
        selectedCharIndex++;
    }

    void enterText(const char* text) {
        if (!text) return;
        int index = MAX(MIN(selectedCharIndex, activeMessage.size()), 0);
        int textLen = strlen(text);
        activeMessage.insert(index, text);
        selectedCharIndex += textLen;
    }

    CommandInput handleKeypress(SDL_Keycode keycode, ArrayRef<Command> possibleCommands) {
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
                if (selectedCharIndex > 0) {
                    activeMessage.erase(MIN(selectedCharIndex-1, activeMessage.size()-1), 1);
                    selectedCharIndex--;
                }
            }
            break;
        case SDLK_TAB:
            insertChar('\t');
            break;
        case SDLK_UP:
            moveLoggedMessage(1);
            break;
        case SDLK_DOWN:
            moveLoggedMessage(-1);
            break;
        case SDLK_LEFT:
            selectedCharIndex = MAX(selectedCharIndex-1, 0);
            break;
        case SDLK_RIGHT:
            selectedCharIndex = MIN(selectedCharIndex+1, activeMessage.size());
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
    Console console = {};

    Gui() {}

    void draw(GuiRenderer& renderer, const FRect& viewport, glm::vec2 mousePosition, const Player* player, const ItemManager& itemManager) {
        area.size = 0;
        SDL_FRect hotbarArea = hotbar.draw(renderer, player, itemManager);
        const ItemStack* heldItemStack = player->heldItemStack;
        if (heldItemStack && !heldItemStack->empty())
            drawHeldItemStack(renderer, itemManager, *heldItemStack, mousePosition);
        area.push(hotbarArea);
    }

    void drawHeldItemStack(GuiRenderer& renderer, const ItemManager& itemManager, const ItemStack& itemStack, glm::vec2 pos);

    bool pointInArea(glm::vec2 point) const {
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