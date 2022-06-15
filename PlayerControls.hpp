#ifndef PLAYER_CONTROLS_INCLUDED
#define PLAYER_CONTROLS_INCLUDED

#include <vector>
#include <functional>
#include "NC/SDLContext.h"
#include <SDL2/SDL.h>
#include "GameState.hpp"
#include "Debug.hpp"
#include "Metadata.hpp"
#include "GameViewport.hpp"
#include "NC/physics.h"
#include "GUI.hpp"
#include "Entities/Entities.hpp"
#include "Entities/Methods.hpp"
#include "Log.hpp"

struct Context;

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
    virtual void updateKeyState(bool isDown, Context* param) {}
};
 
class FunctionKeyBinding : public KeyBinding {
    std::function<void(Context*)> function;
    bool keyWasDown;
public:
    FunctionKeyBinding(char key, std::function<void(Context*)> function) {
        this->function = function;
        this->key = key;
        keyWasDown = false;
    }

    void updateKeyState(bool isDown, Context* param) {
        if (!keyWasDown && isDown) {
            function(param);
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

    void updateKeyState(bool isDown, Context* param) {
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

    void updateKeyState(bool isDown, Context* param) {
        *value = isDown && !*value;
    }
};

MouseState getMouseState();

class PlayerControls {
const GameViewport& gameViewport;
MouseState mouse;
Vec2 mouseWorldPos;
const Uint8* keyboardState;
std::vector<KeyBinding*> keyBindings;
public:
    PlayerControls(const GameViewport& gameViewport): gameViewport(gameViewport) {
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
        mouseWorldPos = gameViewport.pixelToWorldPosition(mouse.x, mouse.y);
    }

    void addKeyBinding(KeyBinding* keyBinding) {
        keyBindings.push_back(keyBinding);
    }

    const MouseState& getMouse() {
        return mouse;
    }

    bool pixelInWorld(int x, int y, const GUI* gui) {
        Vec2 worldPos = gameViewport.pixelToWorldPosition(x, y);
        SDL_Point pos = {x, y};
        bool clickInDisplay = SDL_PointInRect(&pos, &gameViewport.display);
        bool clickOnGUI = gui->pointInArea(pos);
        bool clickInWorld = clickInDisplay && !clickOnGUI;
        return clickInWorld;
    }

    void playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState, GameState* state, const GUI* gui) {
        Vec2 newWorldPos = gameViewport.pixelToWorldPosition(mouseState.x, mouseState.y);
        Vec2 prevWorldPos = gameViewport.pixelToWorldPosition(prevMouseState.x, prevMouseState.y);

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
        if (SDL_PointInRect(&mousePos, &gameViewport.display)) {
            SDL_Point relMousePos = {mousePos.x - gameViewport.display.x, mousePos.y - gameViewport.display.y};
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
        Vec2 worldPos = gameViewport.pixelToWorldPosition(mousePos.x, mousePos.y);
        bool clickInDisplay = SDL_PointInRect(&mousePos, &gameViewport.display);
        bool clickOnGUI = gui->pointInArea(mousePos);
        bool clickInWorld = clickInDisplay && !clickOnGUI;
        Tile* selectedTile = getTileAtPosition(state->chunkmap, worldPos);
        if (event.button == SDL_BUTTON_LEFT) {

            // make sure mouse is within display viewport
            //if ((relativeMousePos.x >= 0 && relativeMousePos.x <= gameViewport.display.w) &&
            //    (relativeMousePos.y >= 0 && relativeMousePos.y <= gameViewport.display.h)) {
            if (clickInDisplay) {
                // first check for GUI clicks
                if (clickOnGUI) {
                    // click counted as on gui, not world.
                    // trigger gui stuff
                    // find clicked hotbar slot (if it was a click on a hotbar slot)
                    for (int slot = 0; slot < NUM_HOTBAR_SLOTS; slot++) {
                        SDL_FRect slotRect = gui->hotbar.slots[slot];
                        if (mousePos.x > slotRect.x && mousePos.x < slotRect.x + slotRect.w &&
                            mousePos.y > slotRect.y && mousePos.y < slotRect.y + slotRect.h) {
                            // click was on slot
                            ItemStack* stack = &state->player.inventory()->items[slot];
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
                    
                }
            }

        } else if (event.button == SDL_BUTTON_RIGHT) {
            static int frame = Metadata.ticks();
            if (clickInWorld) {
                Entity tileEntity;
                if (findTileEntityAtPosition(state, worldPos, &tileEntity)) {
                    Log("Removing entity at %d,%d", (int)floor(worldPos.x), (int)floor(worldPos.y));
                        removeEntityOnTile(&state->ecs, selectedTile);
                }

                if (state->player.heldItemStack) {
                    if (ItemData[state->player.heldItemStack->item].flags &     Items::Usable) {
                        state->player.tryThrowGrenade(&state->ecs, worldPos);
                    }
                }
            }
        }
    }

    void handleKeydown(const SDL_KeyboardEvent& event, GameState* state) {
        switch (event.keysym.sym) {
            case 'e': {
                EntityType<> tileEntity;
                if (findTileEntityAtPosition(state, mouseWorldPos, &tileEntity.toEntity())) {
                    // does entity have inventory
                    if (tileEntity.Has<InventoryComponent>()) {
                        // then open inventory
                        Inventory* inventory = &state->ecs.Get<InventoryComponent>(tileEntity)->inventory;
                        // for now since I dont want to make GUI so just give the items in the inventory to the player
                        for (int i = 0; i < inventory->size; i++) {
                            if (state->player.inventory()) {
                                int nAdded = state->player.inventory()->addItemStack(inventory->items[i]);
                                if (nAdded > 0)
                                    Log("nadded: %d", nAdded);
                                inventory->items[i].reduceQuantity(nAdded);
                            }
                        }
                    }
                }
                break;}
            case 'i': {
                Tile* tile = getTileAtPosition(state->chunkmap, mouseWorldPos);
                EntityType<> tileEntity = tile->entity.cast<>();
                if (tile)
                if (!tileEntity.IsAlive()) {
                    Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
                    Tile* inputTile = getTileAtPosition(state->chunkmap, inputPos);
                    Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
                    Tile* outputTile = getTileAtPosition(state->chunkmap, outputPos);
                    Entity inserter = Entities::Inserter(&state->ecs, mouseWorldPos.vfloor(), 1, inputTile, outputTile);
                    
                    if (tile) {
                        placeEntityOnTile(&state->ecs, tile, inserter);
                    }
                }
                break;}
            case 'r': {
                Tile* tile = getTileAtPosition(state->chunkmap, mouseWorldPos);
                if (tile)
                if (state->ecs.EntityLives(tile->entity)) {
                    if (state->ecs.entityComponents(tile->entity.id)[getID<RotatableComponent>()]) {
                        float* rotation = &state->ecs.Get<RotationComponent>(tile->entity)->degrees;
                        auto rotatable = state->ecs.Get<RotatableComponent>(tile->entity);
                        // left shift switches direction
                        if (keyboardState[SDL_SCANCODE_LSHIFT]) {
                            *rotation -= rotatable->increment;
                        } else {
                            *rotation += rotatable->increment;
                        }
                        rotatable->rotated = true;
                    }
                }
                break;}
            case 'o': {
                auto belt = Entities::TransportBelt(&state->ecs, mouseWorldPos.vfloor());
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
            case SDLK_SPACE: {
                break;}
            default:
                break;
        }

        for (int i = 1; i < 10; i++) {
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

        player->setPosition(newPlayerPos);
    }

    void update(GameState* state, const GUI* gui, Context* context) {
        Vec2 aimingPosition = gameViewport.pixelToWorldPosition(mouse.x, mouse.y);

        if (mouse.leftButtonDown()) {
            leftMouseHeld(mouse, state, gui);
        }
        if (mouse.rightButtonDown()) {
            rightMouseHeld(mouse, state, gui);
        }

        int sidewaysInput = keyboardState[SDL_SCANCODE_D] - keyboardState[SDL_SCANCODE_A];
        // remember that up is negative and down is positive in the coordinate system
        int updownInput = keyboardState[SDL_SCANCODE_S] - keyboardState[SDL_SCANCODE_W]; 
        // might want to move this constant to constants.hpp
        float speed = PLAYERSPEED;
        if (sidewaysInput && updownInput) {
            speed *= sqrt(2) / 2;
        }

        float movementX = sidewaysInput * speed;
        float movementY = updownInput * speed;

        movePlayer(state, Vec2(movementX, movementY));

        if (keyboardState[SDL_SCANCODE_G]) {
            Tile* selectedTile = getTileAtPosition(state->chunkmap, aimingPosition);
            if (!state->ecs.EntityLives(selectedTile->entity)) {
                Log("Adding entity at %d,%d", (int)floor(aimingPosition.x), (int)floor(aimingPosition.y));
                Entity chest = Entities::Chest(&state->ecs, aimingPosition.vfloor(), 32, 1, 1);
                placeEntityOnTile(&state->ecs, selectedTile, chest);
            }
            
        }

        for (auto keyBinding : keyBindings) {
            keyBinding->updateKeyState(keyboardState[SDL_GetScancodeFromKey(keyBinding->key)], context);
        }
    }
};

#endif