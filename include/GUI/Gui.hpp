#ifndef GUI_INCLUDED
#define GUI_INCLUDED

#include "My/Vec.hpp"
#include "sdl_gl.hpp"
#include "items/items.hpp"
#include "constants.hpp"
#include "Player.hpp"
#include "commands.hpp"
#include "elements.hpp"
#include "update.hpp"
#include "GUI/ecs-gui.hpp"

struct GuiRenderer;

namespace GUI {

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
        {200, 200, 50, 255},
        {255, 0, 0, 255}
    };

    struct LogMessage {
        std::string text;
        SDL_Color color;
        MessageType type;
        bool playerEntered = false;
        int copyNumber = 0;
    };

    std::vector<LogMessage> log;

    std::string activeMessage;
    int recallIndex = -1; // going through the log, to retype and old message, this index is for seeing what message its on. -1 means not using the log history currently
    int selectedCharIndex = 0;

    #define CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION 4.0 // seconds
    double timeLastMessageSent = NAN;
    double timeLastCursorMove = NAN;

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
        recallIndex = -1;
    }

    void newMessage(const char* text, MessageType type) {
        bool playerEntered = type == MessageType::Default || type == MessageType::Command;
        LogMessage message = {
            .text = text,
            .color = typeColors[(int)type],
            .type = type,
            .playerEntered = playerEntered
        };
        if (log.size() > 0) {
        LogMessage& lastMessage = log.back();
            if (lastMessage.text == message.text
                && lastMessage.color.r == message.color.r
                && lastMessage.color.g == message.color.g
                && lastMessage.color.b == message.color.b
                && lastMessage.color.a == message.color.a
                && lastMessage.playerEntered == message.playerEntered)
            {
                lastMessage.copyNumber += 1;
            } else {
               log.push_back(message); 
            }
        } else {
            log.push_back(message);
        }

        timeLastMessageSent = Metadata->seconds();
    }

    void moveCursor(int index) {
        int min = MIN(index, activeMessage.size());
        index = MAX(min, -1);
        selectedCharIndex = index;
        timeLastCursorMove = Metadata->seconds();
    }

    int playerMessageCount() const {
        int count = 0;
        for (auto& message : log) {
            count += message.playerEntered;
        }
        return count;
    }

    // will clamp indices below or above the limit to an empty message or oldest message, respectively
    void recallPastMessage() { 
        if (recallIndex >= (int)log.size() - 1) return;
        int relativeIndex = (int)log.size() - recallIndex - 2; // reverse
        relativeIndex = std::clamp(relativeIndex, 0, (int)log.size() - 1);
        for (int i = relativeIndex; i < log.size(); i++) {
            LogMessage newMessage = log[i];
            if (newMessage.type == MessageType::Default) {
                activeMessage = newMessage.text;
                recallIndex += i - relativeIndex + 1;
                break;
            }
        }
        //if (activeMessage.empty()) LogError("Failed to select logged message!");
        //logHistoryIndex = i; // we set the new index to i, not historyIndex because we might not actually end up on that one,
        // if the index is too great (see method desc)
        moveCursor(activeMessage.size());
    }

    // will clamp indices below or above the limit to an empty message or oldest message, respectively
    void recallBackMessage() {
        if (recallIndex <= 0) return; // already on that index, don't need to do anything
        int relativeIndex = (int)log.size() - recallIndex; // reverse
        relativeIndex = std::clamp(relativeIndex, 0, (int)log.size() - 1);
        for (int i = relativeIndex; i >= 0; i--) {
            LogMessage newMessage = log[i];
            if (newMessage.type == MessageType::Default) {
                activeMessage = newMessage.text;
                recallIndex += i - relativeIndex - 1;
                break;
            }
        }
        if (activeMessage.empty()) LogError("Failed to select logged message!");
        //logHistoryIndex = i; // we set the new index to i, not historyIndex because we might not actually end up on that one,
        // if the index is too great (see method desc)
        moveCursor(activeMessage.size());
    }

    void enterChar(char c) {
        int index = MAX(MIN(selectedCharIndex, activeMessage.size()), 0);
        activeMessage.insert(index, 1, c);
        moveCursor(selectedCharIndex + 1);
    }

    void enterText(const char* text) {
        if (!text) return;
        int index = MAX(MIN(selectedCharIndex, activeMessage.size()), 0);
        int textLen = strlen(text);
        activeMessage.insert(index, text);
        moveCursor(selectedCharIndex + textLen);
    }

    CommandInput handleKeypress(SDL_Keycode keycode, ArrayRef<Command> possibleCommands) {
        CommandInput command = {};
        switch (keycode) {
        case SDLK_RETURN2:
        case SDLK_RETURN: {
            recallIndex = -1;

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
                    moveCursor(selectedCharIndex - 1);
                }
            }
            break;
        case SDLK_TAB:
            enterChar('\t');
        break;
        case SDLK_UP:
            recallPastMessage();
            break;
        case SDLK_DOWN:
            recallBackMessage();
            break;
        case SDLK_LEFT:
            moveCursor(MAX(selectedCharIndex - 1, 0));
            break;
        case SDLK_RIGHT:
            moveCursor(MIN(selectedCharIndex + 1, activeMessage.size()));
            break;
        } // end keycode switch

        return command;
    }

    // the string buffer must be handled (freed) after calling this function
    My::StringBuffer newLogStringBuffer() const {
        auto buffer = My::StringBuffer::WithCapacity(64);
        for (int i = 0; i < log.size(); i++) {
            if (log[i].copyNumber > 0) {
                buffer += string_format("(%d)", + log[i].copyNumber);
            }
            buffer += log[i].text.c_str();
        }
        return buffer;
    }

    void destroy() {
        
    }
};


inline void makeGuiPrototypes(GuiManager& gui) {
    auto& pm = gui.prototypes;

    auto normal = Prototypes::Normal(pm);
    auto epic = Prototypes::Epic(pm);

    pm.add(normal);
    pm.add(epic);
}

struct Gui {
    My::Vec<SDL_FRect> area = My::Vec<SDL_FRect>(0);
    Console console = {};

    GuiManager manager;

    struct MySystems {
        Systems::RenderBackgroundSystem* renderBackgroundSys;
        Systems::SizeConstraintSystem* sizeConstraintSystem;

        void init(Systems::SystemManager& manager, GuiRenderer& renderer) {
            renderBackgroundSys = new Systems::RenderBackgroundSystem(manager, &renderer);
            sizeConstraintSystem = new Systems::SizeConstraintSystem(manager);
        }

        void update(GuiRenderer& renderer) {
           
        }

        void destroy() {
            delete sizeConstraintSystem;
            delete renderBackgroundSys;
        }
    } systems;

    Gui() {}

    void init(GuiRenderer& renderer) {
        My::Vec<GECS::ComponentInfo> componentInfo; // TODO: Unimportant: This is leaked currently
        {
            using namespace GUI::EC;
            constexpr auto componentInfoArray = GECS::getComponentInfoList<GUI_COMPONENTS_LIST>();
            componentInfo = My::Vec<GECS::ComponentInfo>(&componentInfoArray[0], componentInfoArray.size());
        }
        manager = GUI::GuiManager(ArrayRef(componentInfo.data, componentInfo.size), GUI::ElementTypes::Count);
        makeGuiPrototypes(manager);
        initGui(manager);

        systems.init(manager.systemManager, renderer);
    }

    void renderElements(GuiRenderer& renderer, const PlayerControls& playerControls);

    void updateHotbar(const Player& player, const ItemManager& itemManager) {
        auto hotbarElement = manager.getNamedElement("hotbar");

        auto* inventory = player.inventory();

        static char slotText[64][9];

        // my hotbar
        for (int i = 0; i < player.numHotbarSlots; i++) {
            char slotName[64];
            snprintf(slotName, 64, "hotbar-slot-%d", i);
            Element slot = manager.getNamedElement(slotName);

            ItemStack stack = inventory->get(i);

            auto textureEc = manager.getComponent<EC::SimpleTexture>(slot);
            auto textEc = manager.getComponent<EC::Text>(slot);

            auto* displayIec = itemManager.getComponent<ITC::Display>(stack.item);
            ItemQuantity stackSize = items::getStackSize(stack.item, itemManager);

            if (textureEc) {
                textureEc->texture = displayIec ? displayIec->inventoryIcon : TextureIDs::Null;
            }

            if (textEc) {
                if (stack.quantity != 0 && stackSize != 1) {
                    auto str = string_format("%d", stack.quantity);
                    strcpy(slotText[i], str.c_str());
                } else {
                    strcpy(slotText[i], "");
                }
                textEc->text = slotText[i];
            }
        }
    }

    std::vector<GameAction> updateGuiState(const GameState* gameState, const PlayerControls& playerControls) {
        updateHotbar(gameState->player, gameState->itemManager);
        return GUI::update(manager, playerControls);
    }

    void drawConsole(GuiRenderer& renderer) {

        Element consoleElement = manager.getNamedElement("console");
        auto* consoleView = manager.getComponent<EC::ViewBox>(consoleElement);

        float scale = renderer.options.scale;
        bool showLog = console.promptOpen || Metadata->seconds() - console.timeLastMessageSent < CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION;
        if (!showLog && console.activeMessage.empty()) {
            manager.hideElement(consoleElement);
            return;
        } else {
            manager.unhideElement(consoleElement);
        }
        
        Element consoleLogElement = manager.getNamedElement("console-log");
        auto* logViewEc = manager.getComponent<EC::ViewBox>(consoleLogElement);
        if (showLog && logViewEc) {
            manager.unhideElement(consoleElement);
            
            TextFormattingSettings logFormatting{
                .align = TextAlignment::BottomLeft,
                .maxWidth = 300,
                .wrapOnWhitespace = false
            };
            TextRenderingSettings logRenderingSettings {
                .font = Fonts->get("Debug"),
                .scale = Vec2(0.5f)
            };
            float messageSpacing = logRenderingSettings.font->height() * 0.25f;
            Vec2 pos = logViewEc->absolute.min;
            for (int i = console.log.size()-1; i >= 0; i--) {
                const Console::LogMessage& message = console.log[i];
                logRenderingSettings.color = message.color;
                auto result = renderer.renderText(message.text.c_str(), pos, logFormatting, logRenderingSettings, logViewEc->level);
                pos.y += result.rect.h + messageSpacing;
            }
        } else {
            manager.hideElement(consoleLogElement);
        }
        
        SDL_Color terminalTextColor = {255,255,255,255};

        Element consoleTerminal = manager.getNamedElement("console-terminal");
        EC::ViewBox* terminalViewEc = manager.getComponent<EC::ViewBox>(consoleTerminal);
        const std::string& activeMessage = console.activeMessage;

        TextFormattingSettings terminalTextFormatting{
            .align = TextAlignment::BottomLeft,
            .maxWidth = renderer.options.size.x,
            .wrapOnWhitespace = false
        };
        const Font* terminalFont = Fonts->get("Debug");
        float terminalFontScale = 0.75;
        TextRenderingSettings terminalTextRenderSettings{
            .color = terminalTextColor,
            .scale = Vec2(terminalFontScale),
            .font = terminalFont
        };

        bool showTerminal = console.promptOpen;
        if (showTerminal) {
            manager.unhideElement(consoleTerminal);

            Vec2* characterPositions = Alloc<Vec2>(activeMessage.size());
            auto textRect = renderer.renderText(activeMessage.c_str(), terminalViewEc->absolute.min, terminalTextFormatting, terminalTextRenderSettings, terminalViewEc->level, characterPositions).rect;
            terminalViewEc->box = *rectAsBox(&textRect);
            if (terminalViewEc->box.size.y < terminalFont->height() * terminalFontScale) {
                terminalViewEc->box.size.y = terminalFont->height() * terminalFontScale;
            }

            Vec2 selectedCharPos = terminalViewEc->absolute.min;
            int selectedCharIndex = console.selectedCharIndex;
            
            // in the middle case
            if (selectedCharIndex >= 0 && selectedCharIndex < activeMessage.size()) {
                selectedCharPos = characterPositions[selectedCharIndex];
                if (selectedCharIndex > 0) {
                    selectedCharPos = characterPositions[selectedCharIndex-1];
                    selectedCharPos.x += terminalFont->advance(activeMessage.back()) * terminalFontScale;
                }
            // end case
            } else if (activeMessage.size() > 0 && selectedCharIndex == activeMessage.size()) {
                selectedCharPos = characterPositions[activeMessage.size()-1];
                selectedCharPos.x += terminalFont->advance(activeMessage.back()) * terminalFontScale;
            }
            // for case where message is empty
            if (activeMessage.size() > 0) {
                selectedCharPos.y += terminalFont->descender() * terminalFontScale;
            }

            Box cursor = {
                selectedCharPos,
                {renderer.pixels(2.0f), terminalFont->height() * terminalFontScale}
            };
            
            // render cursor
            constexpr double flashDelay = 0.5;
            static double timeTilFlash = flashDelay;
            constexpr double flashDuration = 0.5;
            timeTilFlash -= Metadata->frame.deltaTime / 1000.0; // subtract time in seconds from time
            double secondsSinceCursorMove = Metadata->seconds() - console.timeLastCursorMove;
            if (secondsSinceCursorMove < flashDuration) timeTilFlash = -secondsSinceCursorMove;
            if (timeTilFlash < 0.0) {
                renderer.colorRect(cursor, terminalTextColor, terminalViewEc->level);
            }
        } else {
            manager.hideElement(consoleTerminal);
        }
    }

    void draw(GuiRenderer& renderer, const FRect& viewport, const Player* player, const ItemManager& itemManager, const PlayerControls& playerControls) {
       
        const auto* heldItemStack = &player->heldItemStack;
        if (heldItemStack) {
            const ItemStack* item = heldItemStack->get();
            if (item && !item->empty()) {
                drawHeldItemStack(renderer, itemManager, *item, playerControls.mousePixelPos());
            }
        }

        renderElements(renderer, playerControls);

        drawConsole(renderer);
        
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
        console.destroy();
        manager.destroy();
        systems.destroy();
    }
};

}

using GUI::Gui;

#endif