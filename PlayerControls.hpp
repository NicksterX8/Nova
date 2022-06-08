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
#include "Log.hpp"

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

MouseState getMouseState();

class PlayerControls {
const GameViewport gameViewport;
const MouseState mouse;
Vec2 mouseWorldPos;
const Uint8 * const keyboard;
public:
    PlayerControls(const GameViewport* viewport, MouseState mouse, const Uint8 *keyboard):
    gameViewport(*viewport), mouse(mouse), keyboard(keyboard) {
        mouseWorldPos = gameViewport.pixelToWorldPosition(mouse.x, mouse.y);
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
            Log("mouse screen XY: %d,%d", mousePos.x, mousePos.y);

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
                    
                    Log("Clicked world position: %f,%f", worldPos.x, worldPos.y);
                    
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
            if (frame != Metadata.ticks()) {
                frame = Metadata.ticks();
            } else {
                Log("two clicks in one frame!");
            }
        }
    }

    void handleKeydown(const SDL_KeyboardEvent& event, GameState* state) {
        switch (event.keysym.sym) {
            case 't':
                Debug.settings.drawChunkBorders ^= 1;
                break;
            case 'u':
                Debug.settings.drawEntityRects ^= 1;
                break;
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
            case 'q': {
                state->player.releaseHeldItem();
                break;}
            case 'l': {
                // do airstrikes row by row
                for (int y = -100; y < 100; y += 5) {
                    for (int x = -100; x < 100; x += 5) {
                        Entities::Explosive airstrike = Entities::Airstrike(&state->ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
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
                        if (keyboard[SDL_SCANCODE_LSHIFT]) {
                            *rotation -= rotatable->increment;
                        } else {
                            *rotation += rotatable->increment;
                        }
                        rotatable->rotated = true;
                    }
                }
                break;}
            case 'y': {
                Entity enemy = Entities::Enemy(&state->ecs, mouseWorldPos, state->player.entity);
            break;}

            case 'b': {
                auto belt = Entities::TransportBelt(&state->ecs, mouseWorldPos.vfloor());
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

        /*
        Vec2 c = oldPlayerPos;
        Vec2 d = movement.norm();
        float r = PLAYER_DIAMETER / 2.0f;
        Vec2 p1 = c + (Vec2){r, 0};
        Vec2 p2 = c + (Vec2){0, r};
        Vec2 r1 = p1 + movement;
        Vec2 r2 = p2 + movement;

        int minPlayerTileX = floor(fmin(oldPlayerPos.x, newPlayerPos.x)) - 1;
        int maxPlayerTileX = floor(fmax(oldPlayerPos.x, newPlayerPos.x)) + 1;
        int minPlayerTileY = floor(fmin(oldPlayerPos.y, newPlayerPos.y)) - 1;
        int maxPlayerTileY = floor(fmax(oldPlayerPos.y, newPlayerPos.y)) + 1;

        for (int ty = minPlayerTileY; ty < maxPlayerTileY; ty++) {
            // horizontal line intersects circle
            if (fabs(c.y - ty) > r) {

            }
        }
        */

       player->setPosition(newPlayerPos);
    }
    /*
    void movePlayer2(GameState* state, Vec2 movement) {
        Player* player = &state->player;
        Vec2 oldPlayerPos = {player->x, player->y};
        Vec2 newPlayerPos = oldPlayerPos + movement;

        Tile* tile = getTileAtPosition(state->chunkmap, newPlayerPos);
        IVec2 tilePos = IVec2(floor(newPlayerPos.x), floor(newPlayerPos.y));

        // int playerDirX = (oldPlayerPos.x > newPlayerPos.x) ? 1 : -1;
        // int playerDirY = (oldPlayerPos.y > newPlayerPos.y) ? 1 : -1;
        
        Vec2 moveDirection = movement.norm();
        Vec2 rotated90Direction = {moveDirection.y, -moveDirection.x}; // rotate 90 degrees (y, -x)
        Vec2 circle1Edge1 = rotated90Direction.scaled(PLAYER_DIAMETER / 2.0f);
        Vec2 circle1Edge2 = circle1Edge1.scaled(-1);

        Vec2 circle2Edge1 = circle1Edge1 + movement;
        Vec2 circle2Edge2 = circle1Edge2 + movement;

        Vec2 circleAreaQuad[4] = {
            circle1Edge1,
            circle1Edge2,
            circle2Edge2,
            circle2Edge1
        };

        Polygon circleArea;
        circleArea.nPoints = 4;
        circleArea.p = circleAreaQuad;

        std::vector<Vec2> collidingTiles;

        for (float dY = -1; dY < 1; dY++) {
            for (float dX = -1; dX < 1; dX++) {

                float x = dX + oldPlayerPos.x;
                float y = dY + oldPlayerPos.y;

                int tileX = floor(x);
                int tileY = floor(y);

                Tile* tile = getTile(state->chunkmap, tileX, tileY);

                if (TileTypeData[tile->type].flags & TileTypeFlag::Walkable) {
                    continue;
                }

                int colliding = 0;

                for (int p = 0; p < 4; p++) {
                    colliding |= pointInPoly({(float)tileX,   (float)tileY},   circleArea);
                    colliding |= pointInPoly({(float)tileX+1, (float)tileY},   circleArea);
                    colliding |= pointInPoly({(float)tileX,   (float)tileY+1}, circleArea);
                    colliding |= pointInPoly({(float)tileX+1, (float)tileY+1}, circleArea);

                    if (colliding) {
                        collidingTiles.push_back({dX, dY});
                        break;
                    }
                }
            }
        }

        if (collidingTiles.size() > 0) {
            Vec2 minDistVec;
            float minDist = INFINITY;
            for (int i = 0; i < collidingTiles.size(); i++) {
                Vec2 pos = collidingTiles[i];
                float tDist = pos.x * pos.x + pos.y * pos.y;
                if (tDist < minDist) {
                    minDist = tDist;
                    minDistVec = pos;
                }
            }

            Vec2 collidingTilePosF = minDistVec + oldPlayerPos;

            Vec2 tileCenterDelta = ((Vec2){collidingTilePosF.x + 0.5f, collidingTilePosF.y + 0.5f} - oldPlayerPos);
            
            Vec2 offset = tileCenterDelta.norm().scaled(-M_SQRT2);

            newPlayerPos += offset;
        }

        player->x = newPlayerPos.x;
        player->y = newPlayerPos.y;

    }
    */

    void update(const MouseState& mouse, const Uint8* keyboardState, GameState* state, const GUI* gui) {
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
    }
};

#endif