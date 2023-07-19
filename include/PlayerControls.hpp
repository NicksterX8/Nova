#ifndef PLAYER_CONTROLS_INCLUDED
#define PLAYER_CONTROLS_INCLUDED

#include <vector>
#include <functional>
#include "sdl.hpp"
#include "GameState.hpp"
#include "utils/Debug.hpp"
#include "utils/Metadata.hpp"
#include "GUI/Gui.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/entities/methods.hpp"
#include "utils/Log.hpp"
#include "My/String.hpp"
#include "Camera.hpp"
#include "commands.hpp"

struct Game;
/* Mouse Stuff */

inline Vec2 getMouseWorldPosition(const Camera& camera) {
    SDL_Point pixelPosition = SDL::getMousePixelPosition();
    return camera.pixelToWorld(pixelPosition.x, pixelPosition.y);
}

struct MouseState {
    int x;
    int y;
    Uint32 buttons;

    void updateState(MouseState* mouseState) {
        SDL_Point pos = SDL::getMousePixelPosition();
        mouseState->x = pos.x;
        mouseState->y = pos.y;
        mouseState->buttons = SDL::getMouseButtons();
    }

    bool leftButtonDown() const {
        return buttons & SDL_BUTTON_LMASK;
    }

    bool rightButtonDown() const {
        return buttons & SDL_BUTTON_RMASK;
    }

    bool middleButtonDown() const {
        return buttons & SDL_BUTTON_MMASK;
    }
};

MouseState getMouseState();

/* Keyboard stuff */

struct KeyBinding {
    char key;

    virtual ~KeyBinding() = default;
    virtual void updateKeyState(bool isDown) {}
};

using FunctionKeyBindingCallback = std::function<void()>;

class FunctionKeyBinding : public KeyBinding {
    FunctionKeyBindingCallback function;
    bool keyWasDown;
public:
    FunctionKeyBinding(char key, FunctionKeyBindingCallback function) {
        this->function = function;
        this->key = key;
        keyWasDown = false;
    }

    void updateKeyState(bool isDown) {
        if (!keyWasDown && isDown) {
            function();
        }
        keyWasDown = isDown;
    }
};

template<class T>
class ToggleKeyBinding : public KeyBinding {
    T* value;
    bool keyWasDown;
public:
    ToggleKeyBinding(char key, T* value): value(value) {
        this->key = key;
        keyWasDown = false;
    }

    void updateKeyState(bool isDown) {
        if (isDown) {
            if (!keyWasDown) {
                *value = !*value;
            }
        }
        keyWasDown = isDown;
    }
};

template<class T>
struct ClickKeyBinding : public KeyBinding {
    T* value;

    ClickKeyBinding(char key, T* value): value(value) {
        this->key = key;
    }

    void updateKeyState(bool isDown) {
        *value = isDown && !*value;
    }
};

struct Keyboard {
    ArrayRef<Uint8> keyState; // index with SDL_Scancode to get a 1 for key being pressed, 0 otherwise
    My::Vec<KeyBinding*> bindings;

    Keyboard() {
        int keyArrayLength;
        const Uint8* state = SDL_GetKeyboardState(&keyArrayLength);
        keyState = ArrayRef<Uint8>(state, keyArrayLength);
        bindings = My::Vec<KeyBinding*>::Empty();
    }

    void destroy() {
        for (auto binding : bindings) {
            delete binding;
        }
        bindings.destroy();
    }
};

/* Controller Stuff */

struct Controller {
    static constexpr const char* TypeNames[] = {
        "unknown",
        "Xbox 360",
        "Xbox One",
        "PS3",
        "PS4",
        "Nintendo Switch Pro",
        "virtual",
        "PS5",
        "Amazon Luna",
        "Google Stadia"
    };

    int joystickIndex = 0;
    SDL_GameController* gameController = nullptr;

    SDL_GameControllerType type = SDL_CONTROLLER_TYPE_UNKNOWN;
    const char* controllerName = TypeNames[SDL_CONTROLLER_TYPE_UNKNOWN];

    float leftStickDeadzone = 0.15f;
    float rightStickDeadzone = 0.05f;

    Controller() {}

    Controller(int joystick_index) {
        connect(joystick_index);
    }

    bool connect(int joystick_index) {
        if (connected()) disconnect();

        int numJoysticks = SDL_NumJoysticks();
        if (numJoysticks) {
            gameController = SDL_GameControllerOpen(0);
            joystickIndex = 0;
            if (gameController) {
                type = SDL_GameControllerGetType(gameController);
                controllerName = TypeNames[type];
                LogInfo("Connected %s controller with joystick index %d.", controllerName, joystickIndex);
            } else {
                LogError("Failed to open game controller - Error: %s", SDL_GetError());
            }
        }
        return false;
    }
    
    bool connected() const {
        return gameController != nullptr;
    }

    void disconnect()  {
        SDL_GameControllerClose(gameController);
        gameController = nullptr;
    }
};

/* Main class */

class PlayerControls {
public:
    MouseState mouse;
    Vec2 mouseWorldPos;
    Keyboard keyboard = {};
    Controller controller = {};
    
    Camera& camera;

    bool enteringText;

    bool connectController() {
        bool success = controller.connect(0);
        return success;
    }

    PlayerControls(Camera& camera): camera(camera) {
        mouse = getMouseState();
        mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));

        connectController();
       
        enteringText = false;
    }

    Sint16 getAxisRaw(SDL_GameControllerAxis axis) const {
        return SDL_GameControllerGetAxis(controller.gameController, axis);
    }

    float getNormalizedAxis(SDL_GameControllerAxis axis) const {
        return (float)getAxisRaw(axis) / INT16_MAX;
    }

    Vec2 getLeftStickVector() const {
        Sint16 x = getAxisRaw(SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 y = getAxisRaw(SDL_CONTROLLER_AXIS_LEFTY);

        float xf = (float)x / INT16_MAX;
        float yf = (float)-y / INT16_MAX;

        if (abs(xf) < controller.leftStickDeadzone) xf = 0.0f;
        if (abs(yf) < controller.leftStickDeadzone) yf = 0.0f;

        return {xf, yf};
    }

    static float applyDeadzone(float x, float deadzone) {
        return MAX(abs(x) - deadzone, 0) * ((int)signbit(x) * -2 + 1) ;
    }

    Vec2 getRightStickVector() const {
        float x = getNormalizedAxis(SDL_CONTROLLER_AXIS_RIGHTX);
        float y = getNormalizedAxis(SDL_CONTROLLER_AXIS_RIGHTY);

        x = applyDeadzone(x, controller.rightStickDeadzone);
        y = applyDeadzone(y, controller.rightStickDeadzone);
        return {x, y};
    }

    void destroy() {
        keyboard.destroy();
        controller.disconnect();
    }
    
    /* 
    * Update the internal state of this class, like mouse position.
    * Call near the beginning of the frame, before using any methods of this class,
    * to prevent using outdated state.
    */
    void updateState() {
        mouse = getMouseState();
        mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));
    }

    void addKeyBinding(KeyBinding* keyBinding) {
        keyboard.bindings.push(keyBinding);
    }

    bool pixelInWorld(int x, int y, const Gui* gui) {
        SDL_Point pos = {x, y};
        bool clickInDisplay = SDL_PointInRect(&pos, &camera.displayViewport);
        bool clickOnGui = gui->pointInArea(pos);
        return clickInDisplay && !clickOnGui;
    }

    void playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState, GameState* state, const Gui* gui) {
        Vec2 newWorldPos = camera.pixelToWorld(mouseState.x, mouseState.y);
        Vec2 prevWorldPos = camera.pixelToWorld(prevMouseState.x, prevMouseState.y);

        if (mouseState.buttons & SDL_BUTTON_LMASK) {
            // only do it if the isnt in the gui either currently or previously
            if (!gui->pointInArea({mouseState.x, mouseState.y}) || !gui->pointInArea({prevMouseState.x, prevMouseState.y})) {
                auto line = raytraceDDA(prevWorldPos, newWorldPos);
                ScopedAlloc(Tile, tiles, line.size());
                getTiles(state->chunkmap, line.data(), tiles, line.size());
                auto heldItemStack = state->player.heldItemStack;
                if (heldItemStack && state->player.canPlaceItemStack(*heldItemStack)) {
                    for (int i = 0; i < line.size(); i++) {
                        Tile* tile = getTileAtPosition(state->chunkmap, line[i]);
                        if (tile) {
                            state->player.tryPlaceItemStack(*heldItemStack, tile, state->itemManager);
                        }
                    }
                }
            } 
        }
    }

    void placeItem(Item item, GameState* state, Vec2 at) {
        Tile* tile = getTileAtPosition(state->chunkmap, at);
        if (tile) {
            //state->player.tryPlaceItem(tile);
        }
    }

    void leftMouseHeld(const MouseState& mouse, GameState* state, const Gui* gui) {
        SDL_Point mousePos = {mouse.x, mouse.y};
        if (SDL_PointInRect(&mousePos, &camera.displayViewport)) {
            SDL_Point relMousePos = {mousePos.x - camera.displayViewport.x, mousePos.y - camera.displayViewport.y};
            if (!gui->pointInArea(relMousePos)) {
                // do something
            }
        }
    }

    void rightMouseHeld(const MouseState& mouse, GameState* state, const Gui* gui) {
        if (pixelInWorld(mouse.x, mouse.y, gui)) {
            state->player.tryShootSandGun(&state->ecs, mouseWorldPos);
        }
    }

    void handleMouseMotion(const SDL_MouseMotionEvent& event, GameState* state) {
        
    }

    void clickOnEntity(Entity clickedEntity, GameState* state) {
        // if the entity is already selected, deselect it, otherwise select the new one
        if (clickedEntity != state->player.selectedEntity)
            state->player.selectedEntity = clickedEntity;
        else
            state->player.selectedEntity = NullEntity;
        
        OptionalEntity<> selectedEntity = state->player.selectedEntity;
        if (selectedEntity.Has<EC::Render, EC::EntityTypeEC>(&state->ecs)) {
            auto e = selectedEntity;
            const char* name = selectedEntity.Get<EC::EntityTypeEC>(&state->ecs)->name;
            LogInfo("name: %s", name);
            auto* pos = state->ecs.Get<EC::Position>(e);
            auto* size = state->ecs.Get<EC::Size>(e);
            if (pos) {
                LogInfo("position: %.1f,%.1f", pos->x, pos->y);
            }
            if (size) {
                LogInfo("size: %.1f,%.1f", size->width, size->height);
            }
        }
        if (selectedEntity.Has<EC::Grabable, EC::ItemStack>(&state->ecs)) {
            ItemStack itemGrabbed = selectedEntity.Get<EC::ItemStack>(&state->ecs)->item;
            state->player.inventory()->addItemStack(itemGrabbed);
            state->ecs.Destroy(selectedEntity);
        }
    }

    void handleClick(const SDL_MouseButtonEvent& event, GameState* state, const Gui* gui) {
        // mouse event coordinates are reported in pixel points, which are not representative of actual pixels
        // on high DPI displays, so we scale it by the pixel scale to get actual pixels.
        SDL_Point mousePos = {(int)(event.x * SDL::pixelScale), (int)(event.y * SDL::pixelScale)};
        Vec2 worldPos = camera.pixelToWorld(mousePos.x, mousePos.y);
        bool clickInDisplay = SDL_PointInRect(&mousePos, &camera.displayViewport);
        bool clickOnGui = gui->pointInArea(mousePos);
        bool clickInWorld = clickInDisplay && !clickOnGui;
        Tile* selectedTile = getTileAtPosition(state->chunkmap, worldPos);
        if (event.button == SDL_BUTTON_LEFT) {

            // make sure mouse is within display viewport
            if (clickInDisplay) {
                // first check for GUI clicks
                if (clickOnGui) {
                    // click counted as on gui, not world.
                    // trigger gui stuff
                    // find clicked hotbar slot (if it was a click on a hotbar slot)
                    for (int slot = 0; slot < state->player.numHotbarSlots; slot++) {
                        // TODO: gui slot collision does nothing right now
                        SDL_FRect slotRect = gui->hotbar.slots[slot];
                        if (mousePos.x > slotRect.x && mousePos.x < slotRect.x + slotRect.w &&
                            mousePos.y > slotRect.y && mousePos.y < slotRect.y + slotRect.h) {
                            // click was on slot
                            ItemStack* stack = &state->player.inventory()->get(slot);
                            if (state->player.heldItemStack) {
                                // set held stack down in slot only if actually holding stack and if slot is empty
                                if (stack->empty()) {
                                    *stack = *state->player.heldItemStack;
                                    *state->player.heldItemStack = ItemStack::None();
                                    state->player.heldItemStack = nullptr;
                                } else {
                                    // stop holding item but dont move anything
                                    state->player.releaseHeldItem();
                                }
                            } else {
                                state->player.pickupItem(stack);
                                state->player.selectHotbarSlot(slot);
                            }
                            break;
                        }
                    }
                } else {

                    ItemStack* heldItemStack = state->player.heldItemStack;
                    // if the held item stack exists and is placeable, place it
                    if (heldItemStack && heldItemStack->item.has<ITC::Proto::Placeable>()) {
                        placeItem(heldItemStack->item, state, worldPos);
                    } else {
                        state->player.heldItemStack = NULL;
                    }

                    // find entity to select
                    auto entityOnMouse = findPlayerFocusedEntity(&state->ecs, state->chunkmap, mouseWorldPos);
                    if (entityOnMouse.NotNull()) {
                        clickOnEntity(entityOnMouse, state);
                    }
                    
                }
            }

        } else if (event.button == SDL_BUTTON_RIGHT) {
            if (clickInWorld) {
                auto tileEntity = findTileEntityAtPosition(state, worldPos);
                if (tileEntity.Exists(&state->ecs)) {
                    LogInfo("Removing entity at %d,%d", (int)floor(worldPos.x), (int)floor(worldPos.y));
                        removeEntityOnTile(&state->ecs, selectedTile);
                }

                ItemStack* heldItemStack = state->player.heldItemStack;
                if (heldItemStack) {
                    if (heldItemStack->item.has<ITC::Usable>()) {
                        state->player.tryThrowGrenade(&state->ecs, worldPos);
                        auto usable = items::getComponent<ITC::Usable>(heldItemStack->item, state->itemManager);
                        auto onUse = usable->onUse;
                        if (onUse)
                            onUse();
                    }
                }
            }
        }
    }

    void handleCommandInput(CommandInput commandInput, Gui* gui) {
        if (!commandInput.name.empty()) {
            // command entered
            std::string message;
            bool foundCommand = false;
            for (auto& command : gCommands) {
                if (My::streq(command.name, commandInput.name.c_str(), command.nameLength, commandInput.name.size())) {
                    auto result = executeCommand(&command, commandInput.arguments.c_str());
                    message = result.message;
                    foundCommand = true;
                }
            }
            if (!foundCommand) {
                char messageBuf[256];
                snprintf(messageBuf, 256, "Invalid command \"%s\"", commandInput.name.c_str());
                message = std::string(messageBuf);
            }

            gui->console.newMessage(message.c_str(), GUI::Console::MessageType::CommandResult);
        }
    }


    void handleKeydown(const SDL_KeyboardEvent& event, GameState* state, Gui* gui) {
        const SDL_Keycode keycode = event.keysym.sym;
        const SDL_Scancode scancode = event.keysym.scancode;

        if (enteringText) {
            auto commandInput = gui->console.handleKeypress(keycode, gCommands);
            handleCommandInput(commandInput, gui);
        } else {
            switch (keycode) {
                case 'r': {
                    
                break;}
                case 'e': {
                    auto tileEntity = findTileEntityAtPosition(state, mouseWorldPos);
                    if (tileEntity != NullEntity) {
                        if (tileEntity.Has<EC::Inventory>(&state->ecs)) {
                            // then open inventory
                            Inventory& inventory = state->ecs.Get<EC::Inventory>(tileEntity)->inventory;
                            // for now since I dont want to make GUI so just give the items in the inventory to the player
                            for (Uint32 i = 0; i < inventory.size; i++) {
                                if (state->player.inventory()) {
                                    Uint32 numItemsAdded = state->player.inventory()->addItemStack(inventory[i]);
                                    inventory[i].reduceQuantity(numItemsAdded);
                                }
                            }
                        }
                    }
                break;}
                case 'o': {
                    auto belt = Entities::TransportBelt(&state->ecs, Vec2(floor(mouseWorldPos.x), floor(mouseWorldPos.y)));
                    // TODO: entity collision stuff
                break;}
                case 'z': {
                    ItemStack* heldItemStack = state->player.heldItemStack;
                    if (heldItemStack && !heldItemStack->empty()) {
                        ItemStack dropStack = ItemStack(heldItemStack->item, 1);
                        heldItemStack->reduceQuantity(1);
                        Entities::ItemStack(&state->ecs, mouseWorldPos, dropStack, state->itemManager);
                    }
                break;} 
                case 'h': {
                    static bool wireframeModeEnabled = false;
                    if (!wireframeModeEnabled)
                        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    else
                        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    wireframeModeEnabled ^= 1;
                break;}
            }

            for (int i = 1; i <= (int)state->player.numHotbarSlots; i++) {
                if (event.keysym.sym == i + '0') {
                    state->player.selectHotbarSlot(i - 1);
                }
            }
        }

        switch (keycode) {
            case SDLK_RETURN:
                enteringText = !enteringText;
                gui->console.showLog = enteringText;
                gui->console.promptOpen = enteringText;
                break;
            case SDLK_SLASH:
                if (!enteringText) {
                    enteringText = true;
                    gui->console.showLog = true;
                    gui->console.promptOpen = true;
                }
                break;
            default:
                break;
        }
    }

    void handleEvent(const SDL_Event* event, GameState* state, Gui* gui) {
        switch (event->type) {
        case SDL_KEYDOWN:
            handleKeydown(event->key, state, gui);
            break;
        case SDL_MOUSEBUTTONDOWN:
            handleClick(event->button, state, gui);
            break;
        case SDL_MOUSEMOTION:
            handleMouseMotion(event->motion, state);
            break;
        case SDL_TEXTINPUT: {
            const char* text = event->text.text;
            if (enteringText) {
                gui->console.enterText(text);
            }
            break;
        }
        default:
            break;
        }
    }

    void movePlayer(GameState* state, Vec2 movement) {
        Player* player = &state->player;
        Vec2 oldPlayerPos = state->player.getPosition();
        Vec2 newPlayerPos = oldPlayerPos + movement;

        player->setPosition(state->chunkmap, newPlayerPos);
    }

    void doPlayerMovementTick(GameState* state) {
        if (enteringText) return; // dont move player while typing... duh

        int sidewaysInput = keyboard.keyState[SDL_SCANCODE_D] - keyboard.keyState[SDL_SCANCODE_A];
        int updownInput = keyboard.keyState[SDL_SCANCODE_W] - keyboard.keyState[SDL_SCANCODE_S];
        int rotationInput = keyboard.keyState[SDL_SCANCODE_E] - keyboard.keyState[SDL_SCANCODE_Q];

        if (state->player.entity.Has<EC::Rotation, EC::Position>(&state->ecs)) {
            auto playerRotation = state->player.entity.Get<EC::Rotation>(&state->ecs);
            if (sidewaysInput || updownInput) {
                glm::vec2 moveVector = glm::normalize(glm::vec2(sidewaysInput, updownInput));
                float angle = glm::radians(playerRotation->degrees);
                glm::vec2 movement = glm::vec2(
                    moveVector.x * cos(angle) - moveVector.y * sin(angle),
                    moveVector.x * sin(angle) + moveVector.y * cos(angle)
                ) * PLAYER_SPEED;

                movePlayer(state, {movement.x, movement.y});
            }

            
            playerRotation->degrees += rotationInput * PLAYER_ROTATION_SPEED;
            camera.setAngle(playerRotation->degrees);
        }

        Vec2 leftStick = getLeftStickVector();
        movePlayer(state, leftStick);
    }

    void update(SDL_Window* window, GameState* state, const Gui* gui) {
        Vec2 aimingPosition = camera.pixelToWorld({mouse.x, mouse.y});

        if (mouse.leftButtonDown()) {
            leftMouseHeld(mouse, state, gui);
        }
        if (mouse.rightButtonDown()) {
            rightMouseHeld(mouse, state, gui);
        }

        if (keyboard.keyState[SDL_SCANCODE_G]) {
            Tile* selectedTile = getTileAtPosition(state->chunkmap, aimingPosition);
            // TODO: entity collision stuff
            if (selectedTile && false) {
                Entity chest = Entities::Chest(&state->ecs, {floor(aimingPosition.x), floor(aimingPosition.y)}, 32, 1, 1, state->itemManager);
                placeEntityOnTile(&state->ecs, selectedTile, chest);
            }
        }
        
        for (auto keyBinding : keyboard.bindings) {
            keyBinding->updateKeyState(keyboard.keyState[SDL_GetScancodeFromKey(keyBinding->key)]);
        }

        Vec2 rightStick = getRightStickVector();
        if (rightStick.x != 0.0f || rightStick.y != 0.0f) {
            int dx = (int)round(rightStick.x * 12);
            int dy = (int)round(rightStick.y * 12);
            SDL_WarpMouseInWindow(window, mouse.x + dx, mouse.y + dy);
        }
    }
};

#endif