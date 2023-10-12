#include "PlayerControls.hpp"
#include "Game.hpp"

MouseState getMouseState() {
    int mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDL::pixelScale;
    mouseY *= SDL::pixelScale;
    return {
        mouseX,
        mouseY,
        buttons
    };
}

bool Controller::connect(int joystick_index) {
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

void PlayerControls::handleClick(const SDL_MouseButtonEvent& event, GameState* state, const Gui* gui) {
    // mouse event coordinates are reported in pixel points, which are not representative of actual pixels
    // on high DPI displays, so we scale it by the pixel scale to get actual pixels.
    //SDL_Point mousePos = {(int)(event.x * SDL::pixelScale), (int)(event.y * SDL::pixelScale)};
    //Vec2 worldPos = camera.pixelToWorld(mousePos.x, mousePos.y);
    glm::vec2 mousePos = mousePixelPos();
    bool clickInDisplay = pointInRect(mousePos, camera.displayViewport);
    bool clickOnGui = gui->pointInArea(mousePos);
    bool clickInWorld = clickInDisplay && !clickOnGui;
    Tile* selectedTile = getTileAtPosition(state->chunkmap, mouseWorldPos);
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
                    if (pointInRect(mousePos, slotRect)) {
                        // click was on slot
                        ItemStack* stack = &state->player.inventory()->get(slot);
                        if (state->player.heldItemStack) {
                            // set held stack down in slot only if actually holding stack and if slot is empty
                            if (stack->empty()) {
                                *stack = *state->player.heldItemStack;
                                *state->player.heldItemStack = ItemStack::None();
                                state->player.heldItemStack = nullptr;
                                state->player.selectedHotbarStack = -1;
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
                    placeItem(heldItemStack->item, state, mouseWorldPos);
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
            auto tileEntity = findTileEntityAtPosition(state, mouseWorldPos);
            if (tileEntity.Exists(&state->ecs)) {
                LogInfo("Removing entity at %d,%d", (int)floor(mouseWorldPos.x), (int)floor(mouseWorldPos.y));
                    removeEntityOnTile(&state->ecs, selectedTile);
            }

            ItemStack* heldItemStack = state->player.heldItemStack;
            if (heldItemStack) {
                if (heldItemStack->item.has<ITC::Usable>()) {
                    state->player.tryThrowGrenade(&state->ecs, mouseWorldPos);
                    auto usable = items::getComponent<ITC::Usable>(heldItemStack->item, state->itemManager);
                    auto onUse = usable->onUse;
                    if (onUse)
                        onUse();
                }
            }
        }
    }
}

void PlayerControls::clickOnEntity(Entity clickedEntity, GameState* state) {
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
        auto* box = state->ecs.Get<EC::ViewBox>(e);
        auto* position = state->ecs.Get<EC::Position>(e);
        if (box && position) {
            LogInfo("position: %.1f,%.1f", position->x + box->min.x, position->y + box->min.y);
            LogInfo("size: %.1f,%.1f", box->size.x, box->size.y);
        }
    }
    if (selectedEntity.Has<EC::Grabable, EC::ItemStack>(&state->ecs)) {
        ItemStack itemGrabbed = selectedEntity.Get<EC::ItemStack>(&state->ecs)->item;
        state->player.inventory()->addItemStack(itemGrabbed);
        state->ecs.Destroy(selectedEntity);
    }
}

void PlayerControls::playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState, GameState* state, const Gui* gui) {
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

void PlayerControls::handleKeydown(const SDL_KeyboardEvent& event, GameState* state, Gui* gui) {
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

void PlayerControls::handleCommandInput(CommandInput commandInput, Gui* gui) {
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

void PlayerControls::update(SDL_Window* window, GameState* state, const Gui* gui) {
    if (mouse.leftButtonDown()) {
        leftMouseHeld(mouse, state, gui);
    }
    if (mouse.rightButtonDown()) {
        rightMouseHeld(mouse, state, gui);
    }

    if (keyboard.keyState[SDL_SCANCODE_G]) {
        Tile* selectedTile = getTileAtPosition(state->chunkmap, mouseWorldPos);
        // TODO: entity collision stuff
        if (selectedTile && false) {
            Entity chest = Entities::Chest(&state->ecs, {floor(mouseWorldPos.x), floor(mouseWorldPos.y)}, 32, 1, 1, state->itemManager);
            placeEntityOnTile(&state->ecs, selectedTile, chest);
        }
    }
    
    if (!enteringText) {
        for (auto keyBinding : keyboard.bindings) {
            keyBinding->updateKeyState(keyboard.keyState[SDL_GetScancodeFromKey(keyBinding->key)]);
        }
    }

    Vec2 rightStick = controller.getRightStickVector();
    if (rightStick.x != 0.0f || rightStick.y != 0.0f) {
        int dx = (int)round(rightStick.x * 12);
        int dy = (int)round(rightStick.y * 12);
        SDL_WarpMouseInWindow(window, mouse.x + dx, mouse.y + dy);
    }
}

void PlayerControls::handleEvent(const SDL_Event* event, GameState* state, Gui* gui) {
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

void PlayerControls::doPlayerMovementTick(GameState* state) {
    if (enteringText) return; // dont move player while typing... duh

    g.playerMovement = {0.0f, 0.0f};

    int sidewaysInput = keyboard.keyState[SDL_SCANCODE_D] - keyboard.keyState[SDL_SCANCODE_A];
    int updownInput = keyboard.keyState[SDL_SCANCODE_W] - keyboard.keyState[SDL_SCANCODE_S];
    int rotationInput = keyboard.keyState[SDL_SCANCODE_E] - keyboard.keyState[SDL_SCANCODE_Q];

    if (state->player.entity.Has<EC::Rotation, EC::ViewBox>(&state->ecs)) {
        auto playerRotation = state->player.entity.Get<EC::Rotation>(&state->ecs);
        if (sidewaysInput || updownInput) {
            glm::vec2 moveVector = glm::normalize(glm::vec2(sidewaysInput, updownInput));
            float angle = glm::radians(playerRotation->degrees);
            glm::vec2 movement = glm::vec2(
                moveVector.x * cos(angle) - moveVector.y * sin(angle),
                moveVector.x * sin(angle) + moveVector.y * cos(angle)
            ) * PLAYER_SPEED;

            movePlayer(state, movement);
            g.playerMovement = movement;
        }
        
        playerRotation->degrees += rotationInput * PLAYER_ROTATION_SPEED;
        camera.setAngle(playerRotation->degrees);
    }

    Vec2 leftStick = controller.getLeftStickVector();
    movePlayer(state, leftStick);
}