#ifndef PLAYER_CONTROLS_INCLUDED
#define PLAYER_CONTROLS_INCLUDED

#include <vector>
#include <functional>
#include "sdl.hpp"
#include "GameState.hpp"
#include "utils/Debug.hpp"
#include "utils/Metadata.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/entities/methods.hpp"
#include "utils/Log.hpp"
#include "My/String.hpp"
#include "Camera.hpp"
#include "commands.hpp"
#include "actions.hpp"

struct Game;
/* Mouse Stuff */

inline Vec2 getMouseWorldPosition(const Camera& camera) {
    SDL_FPoint pixelPosition = SDL::getMousePixelPosition();
    return camera.pixelToWorld(pixelPosition.x, pixelPosition.y);
}

struct MouseState {
    Vec2 position;
    Uint32 buttons;

    void updateState(MouseState* mouseState) {
        SDL_FPoint pos = SDL::getMousePixelPosition();
        mouseState->position = {pos.x, pos.y};
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
    ArrayRef<bool> keyState; // index with SDL_Scancode to get a 1 for key being pressed, 0 otherwise
    My::Vec<KeyBinding*> bindings;

    Keyboard() {
        int keyArrayLength;
        const bool* state = SDL_GetKeyboardState(&keyArrayLength);
        keyState = ArrayRef<bool>(state, keyArrayLength);
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
    SDL_Gamepad* gameController = nullptr;

    SDL_GamepadType type = SDL_GAMEPAD_TYPE_STANDARD;
    const char* controllerName = TypeNames[SDL_GAMEPAD_TYPE_STANDARD];

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
        SDL_CloseGamepad(gameController);
        gameController = nullptr;
    }

    Sint16 getAxisRaw(SDL_GamepadAxis axis) const {
        return SDL_GetGamepadAxis(gameController, axis);
    }

    float getNormalizedAxis(SDL_GamepadAxis axis) const {
        return (float)getAxisRaw(axis) / INT16_MAX;
    }

    Vec2 getLeftStickVector() const {
        Sint16 x = getAxisRaw(SDL_GAMEPAD_AXIS_LEFTX);
        Sint16 y = getAxisRaw(SDL_GAMEPAD_AXIS_LEFTY);

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
        float x = getNormalizedAxis(SDL_GAMEPAD_AXIS_RIGHTX);
        float y = getNormalizedAxis(SDL_GAMEPAD_AXIS_RIGHTY);

        x = applyDeadzone(x, rightStickDeadzone);
        y = applyDeadzone(y, rightStickDeadzone);
        return {x, y};
    }
};

/* Main class */

class PlayerControls {
public:
    MouseState mouse;
    bool mouseClicked; // mouse was clicked this update
    Vec2 mouseWorldPos;
    Keyboard keyboard = {};
    Controller controller = {};
    
    Camera& camera;

    bool enteringText;

    bool justGrabbedItem;

    Game* game;

    PlayerControls(Game* game);

    /* Utils */

    bool pixelInWorld(glm::vec2 pixel);

    /* Mouse */

    glm::vec2 mousePixelPos() const {
        return {mouse.position.x, camera.pixelHeight - mouse.position.y};
    }

    std::vector<GameAction> handleClick(const SDL_MouseButtonEvent& event);

    void leftMouseHeld(const MouseState& mouse);

    void rightMouseHeld(const MouseState& mouse);

    void handleMouseMotion(const SDL_MouseMotionEvent& event) {
        
    }

    void clickOnEntity(Entity clickedEntity);

    void playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState);

    /* Keyboard */

    void handleKeydown(const SDL_KeyboardEvent& event);

    void handleCommandInput(CommandInput commandInput);

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
        mouseClicked = false;
        mouseWorldPos = camera.pixelToWorld(mouse.position);
    }

    void update();

    std::vector<GameAction> handleEvent(const SDL_Event* event);

    void destroy() {
        keyboard.destroy();
        controller.disconnect();
    }

    /* Player stuff - This probably shouldn't be in this class but whateva */

    void movePlayer(Vec2 movement);

    void doPlayerMovementTick();

    /* Random */

    void placeItem(ItemStack* item, Vec2 at);
};

#endif