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
#include "systems/basic.hpp"
#include "memory/allocators.hpp"
#include "memory/StackAllocate.hpp"

struct GuiRenderer;
template<typename T, size_t Align>
struct alignas(Align) AlignedStruct : T {
    AlignedStruct(const T& val) : T(val) {}
};

namespace GUI {

struct Console {
    using Char = Text::Char;

    enum class MessageType {
        Default,
        Command,
        CommandResult,
        Error,
        NumTypes
    };

    constexpr static SDL_Color TypeColors[(int)MessageType::NumTypes] = {
        {230, 230, 230, 255},
        {200, 200, 50, 255},
        {150, 150, 30, 255},
        {220, 200, 200, 255}
    };

    constexpr static const char* TypePrefixes[(int)MessageType::NumTypes] = {
        "",
        "",
        "",
        "ERROR: "
    };

    constexpr static SDL_Color TypePrefixColors[(int)MessageType::NumTypes] = {
        {},
        {},
        {},
        {255, 0, 0, 255}
    };

    using Colors = TinyValVector<SDL_Color>;

    struct LogMessage {
        std::string text;
        Colors colors;
        MessageType type;
        bool playerEntered = false;
        int copyNumber = 0;
    };

    struct Text {
        std::string text;
        Colors colors;
    };

    Text getText(const LogMessage& message) const {
        std::string text;
        text.reserve(message.text.size() + 10);
        SDL_Color typeColor = TypeColors[(int)message.type];
        bool useColors = message.colors.size() == message.text.size();
        Colors colors;
        colors.reserve(message.text.size() + 10); // for prefixes and stuff

        // copy number
        if (message.copyNumber > 0) {
            auto copyNumberStr = string_format("(%d) ", message.copyNumber);
            text += copyNumberStr;
            colors.append(copyNumberStr.size(), {255, 255, 255, 255});
        }
        // prefix
        auto prefix = TypePrefixes[(int)message.type];
        colors.append(strlen(prefix), TypePrefixColors[(int)message.type]);
        text += prefix;
        // message
        text += message.text;
        // colors.append(colors.size() + message.text.size(), message.colors);
        if (useColors) {
            colors.append(message.colors);
        } else {
            // fill all remaining with typeColor
            colors.resize(text.size(), typeColor);
        }
        
        return {text, colors};
    }

    std::vector<LogMessage> log;

    std::string activeMessage;
    int recallIndex = -1; // going through the log, to retype and old message, this index is for seeing what message its on. -1 means not using the log history currently
    int selectedCharIndex = 0;

    #define CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION 6.0 // seconds
    double timeLastMessageSent = NAN;
    double timeLastCursorMove = NAN;

    bool showLog = false;
    bool promptOpen = false;

    constexpr static SDL_Color activeTextColor = {255, 255, 255, 255};

    Console() {}

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

    void newMessage(const char* text, MessageType type, ArrayRef<SDL_Color> characterColors = {}) {
        bool playerEntered = type == MessageType::Default || type == MessageType::Command;
        int len = strlen(text);
        Colors colors;
        if (characterColors.empty()) {
            colors = TypeColors[(int)type];
        } else {
            colors = characterColors;
        }
        LogMessage message = {
            .text = std::string(text, len),
            .colors = colors,
            .type = type,
            .playerEntered = playerEntered
        };
        if (log.size() > 0) {
            LogMessage& lastMessage = log.back();
            if (lastMessage.text == message.text
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

    static bool allowedCharacter(Char c) {
        return Font::hasChar(c);
    }

    void enterText(const Char* text) {
        if (!text) return;

        int textLen = strlen(text);
        StackAllocate<Char, 32> textBuf(textLen+1);
        memcpy(textBuf.data(), text, textLen+1);

        // make sure all characters are ascii
        for (int c = 0; c < textLen; c++) {
            if (!allowedCharacter(text[c])) {
                textBuf[c] = ::Text::UnsupportedChar;
            }
        }

        int index = MAX(MIN(selectedCharIndex, activeMessage.size()), 0);
        
        activeMessage.insert(index, textBuf.data());
        moveCursor(selectedCharIndex + textLen);
    }

    CommandInput handleKeypress(SDL_Keycode keycode, ArrayRef<Command> possibleCommands, bool enteringText) {
        CommandInput command = {};
        switch (keycode) {
        case SDLK_RETURN2:
        case SDLK_RETURN: {
            if (!enteringText) {
                // we will be now
                showLog = true;
                promptOpen = true;
            } else {
                // message sent
                showLog = false;
                promptOpen = false;
                
                recallIndex = -1;

                command = processMessage(activeMessage, possibleCommands);
                if (command) {
                    pushActiveMessage(MessageType::Command);
                } else {
                    // not command
                    pushActiveMessage(MessageType::Default);
                }
            }
            break;
        }
        case SDLK_SLASH:
            showLog = true;
            promptOpen = true;
            if (!enteringText) {
                enterText("/");
            }
            break;
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
            enterText("\t");
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

    void destroy() {
        
    }
};


inline void makeGuiPrototypes(GuiManager& gui) {
    auto& pm = gui.prototypes;

    auto normal = new Prototypes::Normal(pm);
    auto epic = new Prototypes::Epic(pm);

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
        Systems::RenderTexturesSystem* textureSys;
        Systems::DoElementUpdatesSystem* updateSys;

        void init(Systems::SystemManager& manager, GuiRenderer& renderer, Game* game) {
            auto& allocator = GlobalAllocators.gameScratchAllocator;
            auto* treeTraversal = NEW(Systems::IBarrier(manager), allocator);
            auto* startRendering = NEW(Systems::IBarrier(manager), allocator);
            auto* elementsComplete = NEW(Systems::IBarrier(manager), allocator);

            updateSys = NEW(Systems::DoElementUpdatesSystem(manager, game), allocator);
            renderBackgroundSys = NEW(Systems::RenderBackgroundSystem(manager, &renderer), allocator);
            sizeConstraintSystem = NEW(Systems::SizeConstraintSystem(manager), allocator);
            textureSys = NEW(Systems::RenderTexturesSystem(manager, &renderer), allocator);

            updateSys->orderAfter(treeTraversal);
            sizeConstraintSystem->orderAfter(updateSys);
            elementsComplete->orderAfter(sizeConstraintSystem);
            startRendering->orderAfter(elementsComplete);
            renderBackgroundSys->orderAfter(startRendering);
            textureSys->orderAfter(renderBackgroundSys);
        }

        void update(GuiRenderer& renderer) {
           
        }

        void destroy() {
            delete sizeConstraintSystem;
            delete renderBackgroundSys;
            delete textureSys;
        }
    } systems;

    Gui() {}

    void init(GuiRenderer& renderer, Game* game) {
        using namespace GUI::EC;
        constexpr static auto componentInfoArray = ECS::getComponentInfoList<GUI_COMPONENTS_LIST>();
        manager.init(ArrayRef(componentInfoArray), GUI::ElementTypes::Count);
        manager.systemManager.entityManager = &manager;
        makeGuiPrototypes(manager);
        initGui(manager);

        systems.init(manager.systemManager, renderer, game);
        ECS::System::setupSystems(manager.systemManager);
    }

    void renderElements(GuiRenderer& renderer, const PlayerControls& playerControls);

    std::vector<GameAction> updateGuiState(const GameState* gameState, const PlayerControls& playerControls) {
        
        return GUI::update(manager, playerControls);
    }

    void drawConsole(GuiRenderer& renderer);

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