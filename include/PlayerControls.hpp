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

    bool connect(int joystick_index);
    
    bool connected() const {
        return gameController != nullptr;
    }

    void disconnect()  {
        SDL_GameControllerClose(gameController);
        gameController = nullptr;
    }

    Sint16 getAxisRaw(SDL_GameControllerAxis axis) const {
        return SDL_GameControllerGetAxis(gameController, axis);
    }

    float getNormalizedAxis(SDL_GameControllerAxis axis) const {
        return (float)getAxisRaw(axis) / INT16_MAX;
    }

    Vec2 getLeftStickVector() const {
        Sint16 x = getAxisRaw(SDL_CONTROLLER_AXIS_LEFTX);
        Sint16 y = getAxisRaw(SDL_CONTROLLER_AXIS_LEFTY);

        float xf = (float)x / INT16_MAX;
        float yf = (float)-y / INT16_MAX;

        if (abs(xf) < leftStickDeadzone) xf = 0.0f;
        if (abs(yf) < leftStickDeadzone) yf = 0.0f;

        return {xf, yf};
    }

    static float applyDeadzone(float x, float deadzone) {
        return MAX(abs(x) - deadzone, 0) * ((int)signbit(x) * -2 + 1) ;
    }

    Vec2 getRightStickVector() const {
        float x = getNormalizedAxis(SDL_CONTROLLER_AXIS_RIGHTX);
        float y = getNormalizedAxis(SDL_CONTROLLER_AXIS_RIGHTY);

        x = applyDeadzone(x, rightStickDeadzone);
        y = applyDeadzone(y, rightStickDeadzone);
        return {x, y};
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

    PlayerControls(Camera& camera): camera(camera) {
        mouse = getMouseState();
        mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));

        connectController();
       
        enteringText = false;
    }

    /* Utils */

    bool pixelInWorld(glm::vec2 pixel, const Gui* gui) {
        bool clickInDisplay = pointInRect(pixel, camera.displayViewport);
        bool clickOnGui = gui->pointInArea(pixel);
        return clickInDisplay && !clickOnGui;
    }

    /* Mouse */

    glm::vec2 mousePixelPos() const {
        glm::vec2 scaled = {mouse.x * SDL::pixelScale, mouse.y * SDL::pixelScale};
        return {scaled.x, camera.pixelHeight - scaled.y};
    }

    void handleClick(const SDL_MouseButtonEvent& event, GameState* state, const Gui* gui);

    void leftMouseHeld(const MouseState& mouse, GameState* state, const Gui* gui) {
        IVec2 mousePos = {mouse.x, mouse.y};
        if (pointInRect(mousePos, camera.displayViewport)) {
            glm::vec2 relMousePos = {mousePos.x - camera.displayViewport.x, mousePos.y - camera.displayViewport.y};
            if (!gui->pointInArea(relMousePos)) {
                // do something
            }
        }
    }

    void rightMouseHeld(const MouseState& mouse, GameState* state, const Gui* gui) {
        auto mousePixel = mousePixelPos();
        if (pixelInWorld(mousePixel, gui)) {
            state->player.tryShootSandGun(&state->ecs, mouseWorldPos);
        }
    }

    void handleMouseMotion(const SDL_MouseMotionEvent& event, GameState* state) {
        
    }

    void clickOnEntity(Entity clickedEntity, GameState* state);

    void playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState, GameState* state, const Gui* gui);

    /* Keyboard */

    void handleKeydown(const SDL_KeyboardEvent& event, GameState* state, Gui* gui);

    void handleCommandInput(CommandInput commandInput, Gui* gui);

    void addKeyBinding(KeyBinding* keyBinding) {
        keyboard.bindings.push(keyBinding);
    }

    /* Controller */

    bool connectController() {
        bool success = controller.connect(0);
        return success;
    }

    /* Misc */

    /* 
    * Update the internal state of this class, e.g. mouse position.
    * Call near the beginning of the frame, before using any methods of this class,
    * to prevent using outdated state.
    */
    void updateState() {
        mouse = getMouseState();
        mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));
    }

    void update(SDL_Window* window, GameState* state, const Gui* gui);

    void handleEvent(const SDL_Event* event, GameState* state, Gui* gui);

    void destroy() {
        keyboard.destroy();
        controller.disconnect();
    }

    /* Player stuff - This probably shouldn't be in this class but whateva */

    void movePlayer(GameState* state, Vec2 movement) {
        Player* player = &state->player;
        auto* oldPlayerPos = state->player.get<EC::Position>();
        if (!oldPlayerPos) {
            LogError("no player position!");
            return;
        }
        Vec2 newPlayerPos = oldPlayerPos->vec2() + movement;

        player->setPosition(state->chunkmap, newPlayerPos);
    }

    void doPlayerMovementTick(GameState* state);

    /* Random */

    void placeItem(Item item, GameState* state, Vec2 at) {
        Tile* tile = getTileAtPosition(state->chunkmap, at);
        if (tile) {
            //state->player.tryPlaceItem(tile);
        }
    }
};

#endif