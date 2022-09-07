#ifndef PLAYER_CONTROLS_INCLUDED
#define PLAYER_CONTROLS_INCLUDED

#include <vector>
#include <functional>
#include "NC/SDLContext.h"
#include <SDL2/SDL.h>
#include "GameState.hpp"
#include "utils/Debug.hpp"
#include "utils/Metadata.hpp"
#include "NC/physics.h"
#include "GUI/GUI.hpp"
#include "Entities/Entities.hpp"
#include "Entities/Methods.hpp"
#include "utils/Log.hpp"

struct MouseState {
    int x;
    int y;
    Uint32 buttons;

    void updateState(MouseState* mouseState) {
        SDL_Point pos = SDLGetMousePixelPosition();
        mouseState->x = pos.x;
        mouseState->y = pos.y;
        mouseState->buttons = SDLGetMouseButtons();
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

class ToggleKeyBinding : public KeyBinding {
    bool* value;
    bool keyWasDown;
public:
    ToggleKeyBinding(char key, bool* value): value(value) {
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

struct ClickKeyBinding : public KeyBinding {
    bool* value;
public:
    ClickKeyBinding(char key, bool* value): value(value) {
        this->key = key;
    }

    void updateKeyState(bool isDown) {
        *value = isDown && !*value;
    }
};

MouseState getMouseState();

OptionalEntity<EC::Position, EC::Size, EC::Render>
findPlayerFocusedEntity(const ComponentManager<EC::Position, const EC::Size, const EC::Render>& ecs, const ChunkMap& chunkmap, Vec2 playerMousePos);

class PlayerControls {
public:
    Camera& camera;
    MouseState mouse;
    Vec2 mouseWorldPos;
    std::vector<KeyBinding*> keyBindings;

    const Uint8* keyboardState;

    PlayerControls(Camera& camera): camera(camera) {
        keyboardState = SDL_GetKeyboardState(NULL);
    }

    ~PlayerControls() {
        for (auto keyBinding : keyBindings) {
            delete keyBinding;
        }
    }

    /* 
    * Update the internal state of this class, like mouse position.
    * Call near the beginning of the frame, before using any methods of this class,
    * to prevent using outdated state.
    */
    void updateState() {
        this->mouse = getMouseState();
        mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));
    }

    void addKeyBinding(KeyBinding* keyBinding) {
        keyBindings.push_back(keyBinding);
    }

    const MouseState& getMouse() {
        return mouse;
    }

    bool pixelInWorld(int x, int y, const GUI* gui) {
        Vec2 worldPos = camera.pixelToWorld(x, y);
        SDL_Point pos = {x, y};
        bool clickInDisplay = SDL_PointInRect(&pos, &camera.displayViewport);
        bool clickOnGUI = gui->pointInArea(pos);
        bool clickInWorld = clickInDisplay && !clickOnGUI;
        return clickInWorld;
    }

    void playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState, GameState* state, const GUI* gui) {
        Vec2 newWorldPos = camera.pixelToWorld(mouseState.x, mouseState.y);
        Vec2 prevWorldPos = camera.pixelToWorld(prevMouseState.x, prevMouseState.y);

        if (mouseState.buttons & SDL_BUTTON_LMASK) {
            // only do it if the isnt in the gui either currently or previously
            if (!gui->pointInArea({mouseState.x, mouseState.y}) || !gui->pointInArea({prevMouseState.x, prevMouseState.y})) {
                int lineSize;
                IVec2* line = worldLine(prevWorldPos, newWorldPos, &lineSize);
                for (int i = 0; i < lineSize; i++) {
                    Tile* tile = getTileAtPosition(state->chunkmap, line[i]);
                    if (tile) {
                        state->player.tryPlaceHeldItem(tile);
                    }
                }
                free(line);
            } 
        }
    }

    void placeHeldItem(GameState* state, Vec2 at) {
        Tile* tile = getTileAtPosition(state->chunkmap, at);
        if (tile) {
            state->player.tryPlaceHeldItem(tile);
        }
    }

    void leftMouseHeld(const MouseState& mouse, GameState* state, const GUI* gui) {
        SDL_Point mousePos = {mouse.x, mouse.y};
        if (SDL_PointInRect(&mousePos, &camera.displayViewport)) {
            SDL_Point relMousePos = {mousePos.x - camera.displayViewport.x, mousePos.y - camera.displayViewport.y};
            if (!gui->pointInArea(relMousePos)) {
                // do something
            }
        }
    }

    void rightMouseHeld(const MouseState& mouse, GameState* state, const GUI* gui) {
        SDL_Point mousePos = {mouse.x, mouse.y};
        if (pixelInWorld(mouse.x, mouse.y, gui)) {
            state->player.tryShootSandGun(&state->ecs, mouseWorldPos);
        }
    }

    void handleMouseMotion(const SDL_MouseMotionEvent& event, GameState* state) {
        
    }

    void handleClick(const SDL_MouseButtonEvent& event, GameState* state, const GUI* gui) {
        // mouse event coordinates are reported in pixel points, which are not representative of actual pixels
        // on high DPI displays, so we scale it by the pixel scale to get actual pixels.
        SDL_Point mousePos = {(int)(event.x * SDLPixelScale), (int)(event.y * SDLPixelScale)};
        Vec2 worldPos = camera.pixelToWorld(mousePos.x, mousePos.y);
        bool clickInDisplay = SDL_PointInRect(&mousePos, &camera.displayViewport);
        bool clickOnGUI = gui->pointInArea(mousePos);
        bool clickInWorld = clickInDisplay && !clickOnGUI;
        Tile* selectedTile = getTileAtPosition(state->chunkmap, worldPos);
        if (event.button == SDL_BUTTON_LEFT) {

            // make sure mouse is within display viewport
            //if ((relativeMousePos.x >= 0 && relativeMousePos.x <= camera.displayViewport.w) &&
            //    (relativeMousePos.y >= 0 && relativeMousePos.y <= camera.displayViewport.h)) {
            if (clickInDisplay) {
                // first check for GUI clicks
                if (clickOnGUI) {
                    // click counted as on gui, not world.
                    // trigger gui stuff
                    // find clicked hotbar slot (if it was a click on a hotbar slot)
                    for (unsigned int slot = 0; slot < state->player.numHotbarSlots; slot++) {
                        //SDL_FRect slotRect = gui->hotbar.slots[slot];
                        SDL_FRect slotRect = {0, 0, 0, 0};
                        if (mousePos.x > slotRect.x && mousePos.x < slotRect.x + slotRect.w &&
                            mousePos.y > slotRect.y && mousePos.y < slotRect.y + slotRect.h) {
                            // click was on slot
                            ItemStack* stack = &state->player.inventory()->get(slot);
                            if (state->player.heldItemStack) {
                                // set held stack down in slot only if actually holding stack and if slot is open
                                if (!stack->item) {
                                    *stack = *state->player.heldItemStack;
                                    state->player.heldItemStack->item = 0;
                                    state->player.heldItemStack = NULL;
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
                    if (heldItemStack && (ItemData[state->player.heldItemStack->item].flags & Items::Placeable)) {
                        // place item
                        placeHeldItem(state, worldPos);
                    } else {
                        state->player.heldItemStack = NULL;
                    }

                    // find entity to select
                    auto entityOnMouse = findPlayerFocusedEntity(&state->ecs, state->chunkmap, mouseWorldPos);
                    if (entityOnMouse != state->player.selectedEntity)
                        state->player.selectedEntity = entityOnMouse;
                    else
                        state->player.selectedEntity = NullEntity;
                    
                    OptionalEntity<> selectedEntity = state->player.selectedEntity;
                    if (selectedEntity.Has<EC::Render, EC::EntityTypeEC>(&state->ecs)) {
                        const char* name = selectedEntity.Get<EC::EntityTypeEC>(&state->ecs)->name;
                        Log("name: %s", name);
                    }
                    if (selectedEntity.Has<EC::Grabable, EC::ItemStack>(&state->ecs)) {
                        ItemStack itemGrabbed = selectedEntity.Get<EC::ItemStack>(&state->ecs)->item;
                        state->player.inventory()->addItemStack(itemGrabbed);
                        state->ecs.Destroy(selectedEntity);
                    }
                }
            }

        } else if (event.button == SDL_BUTTON_RIGHT) {
            static int frame = Metadata->ticks();
            if (clickInWorld) {

                auto tileEntity = findTileEntityAtPosition(state, worldPos);
                if (tileEntity.Exists(&state->ecs)) {
                    Log("Removing entity at %d,%d", (int)floor(worldPos.x), (int)floor(worldPos.y));
                        removeEntityOnTile(&state->ecs, selectedTile);
                }

                ItemStack* heldItemStack = state->player.heldItemStack;
                if (heldItemStack) {
                    if (ItemData[state->player.heldItemStack->item].flags & Items::Usable) {
                        state->player.tryThrowGrenade(&state->ecs, worldPos);
                        auto onUse = ItemData[heldItemStack->item].usable.onUse;
                        if (onUse)
                            onUse();
                    }
                }
            }
        }
    }

    void handleKeydown(const SDL_KeyboardEvent& event, GameState* state) {
        switch (event.keysym.sym) {
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
                Entities::tryOccupyTile(belt, &state->chunkmap, &state->ecs);
            break;}
            case 'z': {
                ItemStack* heldItemStack = state->player.heldItemStack;
                if (heldItemStack && heldItemStack->item && heldItemStack->quantity > 0) {
                    ItemStack dropStack = ItemStack(heldItemStack->item, 1);
                    heldItemStack->reduceQuantity(1);
                    auto itemEntity = Entities::ItemStack(&state->ecs, mouseWorldPos, dropStack);
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
            case SDLK_SPACE: {

            break;}
            default:
                break;
        }

        for (int i = 1; i < (int)state->player.numHotbarSlots; i++) {
            if (event.keysym.sym == i + '0') {
                state->player.selectHotbarSlot(i - 1);
            }
        }
    }

    void handleEvent(const SDL_Event& event, GameState* state, const GUI* gui) {
        if (event.type == SDL_KEYDOWN) {
            handleKeydown(event.key, state);
        } else if (event.type == SDL_MOUSEBUTTONDOWN) {
            handleClick(event.button, state, gui);
        } else if (event.type == SDL_MOUSEMOTION) {
            handleMouseMotion(event.motion, state);
        }
    }

    void movePlayer(GameState* state, Vec2 movement) {
        Player* player = &state->player;
        Vec2 oldPlayerPos = state->player.getPosition();
        Vec2 newPlayerPos = oldPlayerPos + movement;

        player->setPosition(state->chunkmap, newPlayerPos);
    }

    void doPlayerMovementTick(GameState* state) {
        int sidewaysInput = keyboardState[SDL_SCANCODE_D] - keyboardState[SDL_SCANCODE_A];
        int updownInput = keyboardState[SDL_SCANCODE_W] - keyboardState[SDL_SCANCODE_S];

        int rotationInput = keyboardState[SDL_SCANCODE_E] - keyboardState[SDL_SCANCODE_Q];
        // might want to move this constant to constants.hpp
        float speed = PLAYER_SPEED;
        if (sidewaysInput && updownInput) {
            speed *= M_SQRT2 / 2.0;
        }

        float movementX = sidewaysInput * speed;
        float movementY = updownInput * speed;

        if (sidewaysInput || updownInput) {
            glm::vec2 moveVector = glm::normalize(glm::vec2(sidewaysInput, updownInput));
            float angle = glm::radians(state->player.entity.Get<EC::Rotation>(&state->ecs)->degrees);
            glm::vec2 movement = glm::vec2(
                moveVector.x * cos(angle) - moveVector.y * sin(angle),
                moveVector.x * sin(angle) + moveVector.y * cos(angle)
            ) * PLAYER_SPEED;

            movePlayer(state, {movement.x, movement.y});
        }

        auto playerRotation = state->player.entity.Get<EC::Rotation>(&state->ecs);
        playerRotation->degrees += rotationInput * PLAYER_ROTATION_SPEED;
    }

    void update(GameState* state, const GUI* gui) {
        Vec2 aimingPosition = camera.pixelToWorld({mouse.x, mouse.y});

        if (mouse.leftButtonDown()) {
            leftMouseHeld(mouse, state, gui);
        }
        if (mouse.rightButtonDown()) {
            rightMouseHeld(mouse, state, gui);
        }

        if (keyboardState[SDL_SCANCODE_G]) {
            Tile* selectedTile = getTileAtPosition(state->chunkmap, aimingPosition);
            if (selectedTile->entity.Exists(&state->ecs)) {
                Entity chest = Entities::Chest(&state->ecs, {floor(aimingPosition.x), floor(aimingPosition.y)}, 32, 1, 1);
                placeEntityOnTile(&state->ecs, selectedTile, chest);
            }
            
        }

        for (auto keyBinding : keyBindings) {
            keyBinding->updateKeyState(keyboardState[SDL_GetScancodeFromKey(keyBinding->key)]);
        }
    }
};

#endif