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

PlayerControls::PlayerControls(Game* game) : camera(game->camera), game(game) {
    mouse = getMouseState();
    mouseWorldPos = camera.pixelToWorld(glm::vec2(mouse.x, mouse.y));

    connectController();
    
    enteringText = false;
    justGrabbedItem = false;
}

bool PlayerControls::pixelInWorld(glm::vec2 pixel) {
    bool clickInDisplay = pointInRect(pixel, camera.displayViewport);
    bool clickOnGui = game->gui->pointInArea(pixel);
    return clickInDisplay && !clickOnGui;
}

void PlayerControls::handleClick(const SDL_MouseButtonEvent& event) {
    // mouse event coordinates are reported in pixel points, which are not representative of actual pixels
    // on high DPI displays, so we scale it by the pixel scale to get actual pixels.
    //SDL_Point mousePos = {(int)(event.x * SDL::pixelScale), (int)(event.y * SDL::pixelScale)};
    //Vec2 worldPos = camera.pixelToWorld(mousePos.x, mousePos.y);
    glm::vec2 mousePos = mousePixelPos();
    bool clickInDisplay = pointInRect(mousePos, camera.displayViewport);
    bool clickOnGui = game->gui->pointInArea(mousePos);
    bool clickInWorld = clickInDisplay && !clickOnGui;
    Tile* selectedTile = getTileAtPosition(game->state->chunkmap, mouseWorldPos);
    if (event.button == SDL_BUTTON_LEFT) {

        // make sure mouse is within display viewport
        if (clickInDisplay) {
            // first check for GUI clicks
            if (clickOnGui) {
                // click counted as on gui, not world.
                // trigger gui stuff
                // find clicked hotbar slot (if it was a click on a hotbar slot)
                for (int slot = 0; slot < game->state->player.numHotbarSlots; slot++) {
                    SDL_FRect slotRect = game->gui->hotbar.slots[slot];
                    if (pointInRect(mousePos, slotRect)) {
                        // click was on slot
                        ItemStack* stack = &game->state->player.inventory()->get(slot);
                        if (game->state->player.heldItemStack) {
                            // set held stack down in slot only if actually holding stack and if slot is empty
                            if (stack->empty()) {
                                *stack = *game->state->player.heldItemStack.get();
                                *game->state->player.heldItemStack.get() = ItemStack::None();
                                game->state->player.heldItemStack = ItemHold();
                                game->state->player.selectedHotbarStack = -1;
                            } else {
                                bool combined = items::combineStacks(stack, game->state->player.heldItemStack.get(),game->state->itemManager);
                                if (combined) {
                                    
                                } else {
                                    // stop holding item but dont move anything
                                }

                                
                            }
                        } else {
                            game->state->player.pickupItem(*stack);
                            game->state->player.selectHotbarSlot(slot);
                            *stack = ItemStack::None();
                            this->justGrabbedItem = true;
                        }
                        break;
                    }
                }
            } else {
                // World click
                ItemStack* heldItemStack = game->state->player.heldItemStack.get();

                if (game->state->player.isHoldingItem()) {
                    if (heldItemStack->item.has<ITC::Placeable>()) {
                        placeItem(heldItemStack, mouseWorldPos);
                    } 
                    if (heldItemStack->item.has<ITC::Usable>()) {
                        auto usable = items::getComponent<ITC::Usable>(heldItemStack->item, game->state->itemManager);
                        auto edible = items::getComponent<ITC::Edible>(heldItemStack->item, game->state->itemManager);
                        float hg = edible->hungerValue;
                        auto onUse = usable->onUse;
                        if (onUse) {
                            bool success = onUse(game);
                            if (success) {
                                heldItemStack->reduceQuantity(1);
                            }
                        }
                    }

                }

                // find entity to select
                auto entityOnMouse = findPlayerFocusedEntity(&game->state->ecs, game->state->chunkmap, mouseWorldPos);
                if (entityOnMouse.NotNull()) {
                    clickOnEntity(entityOnMouse);
                }
                
            }
        }

    } else if (event.button == SDL_BUTTON_RIGHT) {
        if (clickInWorld) {
            if (game->state->player.isHoldingItem()) {
                game->state->player.releaseHeldItem();
            }

            auto tileEntity = findTileEntityAtPosition(game->state, mouseWorldPos);
            if (tileEntity.Exists(&game->state->ecs)) {
                LogInfo("Removing entity at %d,%d", (int)floor(mouseWorldPos.x), (int)floor(mouseWorldPos.y));
                    removeEntityOnTile(&game->state->ecs, selectedTile);
            }
        }
    }
}

void PlayerControls::leftMouseHeld(const MouseState& mouse) {
    IVec2 mousePos = {mouse.x, mouse.y};
    if (pointInRect(mousePos, camera.displayViewport)) {
        glm::vec2 relMousePos = {mousePos.x - camera.displayViewport.x, mousePos.y - camera.displayViewport.y};
        if (!game->gui->pointInArea(relMousePos)) {
            // do something
        }
    }
}

void PlayerControls::rightMouseHeld(const MouseState& mouse) {
    auto mousePixel = mousePixelPos();
    if (pixelInWorld(mousePixel)) {
        //state->player.tryShootSandGun(&state->ecs, mouseWorldPos);
    }
}

void PlayerControls::clickOnEntity(Entity clickedEntity) {
    Entity* selectedEntity = &game->state->player.selectedEntity;
    // if the entity is already selected, deselect it, otherwise select the new one
    if (clickedEntity == *selectedEntity) {
        game->state->ecs.Remove<EC::Selected>(clickedEntity);
        *selectedEntity = NullEntity;
    } else {
        if (game->state->ecs.EntityHas<EC::Selected>(*selectedEntity)) {
            game->state->ecs.Remove<EC::Selected>(*selectedEntity);
        }
        game->state->ecs.Add<EC::Selected>(clickedEntity, {});
        *selectedEntity = clickedEntity;
    }
    
    if (selectedEntity->Has<EC::Render, EC::EntityTypeEC>(&game->state->ecs)) {
        auto e = *selectedEntity;
        const char* name = e.Get<EC::EntityTypeEC>(&game->state->ecs)->name;
        LogInfo("name: %s | id: %d", name, e.id);
        auto* box = game->state->ecs.Get<EC::ViewBox>(e);
        auto* position = game->state->ecs.Get<EC::Position>(e);
        if (box && position) {
            LogInfo("position: %.1f,%.1f", position->x + box->box.min.x, position->y + box->box.min.y);
            LogInfo("size: %.1f,%.1f", box->box.size.x, box->box.size.y);
        }
    }
    if (selectedEntity->Has<EC::Grabable, EC::ItemStack>(&game->state->ecs)) {
        ItemStack itemGrabbed = selectedEntity->Get<EC::ItemStack>(&game->state->ecs)->item;
        game->state->player.inventory()->addItemStack(itemGrabbed);
        game->state->ecs.Destroy(*selectedEntity);
    }
}

void PlayerControls::playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState) {
    Vec2 newWorldPos = camera.pixelToWorld(mouseState.x, mouseState.y);
    Vec2 prevWorldPos = camera.pixelToWorld(prevMouseState.x, prevMouseState.y);

    if (mouseState.buttons & SDL_BUTTON_LMASK) {
        // only do it if the isnt in the gui either currently or previously
        if (!game->gui->pointInArea({mouseState.x, mouseState.y})) {
            auto line = raytraceDDA(prevWorldPos, newWorldPos);
            ScopedAlloc(Tile, tiles, line.size());
            getTiles(game->state->chunkmap, line.data(), tiles, line.size());
            ItemStack* heldItemStack = game->state->player.heldItemStack.get();
            
            if (!justGrabbedItem) {
                if (heldItemStack && game->state->player.canPlaceItemStack(*heldItemStack)) {
                    for (int i = 0; i < line.size(); i++) {
                        Tile* tile = getTileAtPosition(game->state->chunkmap, line[i]);
                        if (tile) {
                            game->state->player.tryPlaceItemStack(heldItemStack, tile, game->state->itemManager);
                        }
                    }
                }
            }
        } 
    }
}

void PlayerControls::handleKeydown(const SDL_KeyboardEvent& event) {
    const SDL_Keycode keycode = event.keysym.sym;
    const SDL_Scancode scancode = event.keysym.scancode;

    if (enteringText) {
        auto commandInput = game->gui->console.handleKeypress(keycode, gCommands);
        handleCommandInput(commandInput);
    } else {
        switch (keycode) {
            case 'r': {
                
            break;}
            case 'e': {
                auto tileEntity = findTileEntityAtPosition(game->state, mouseWorldPos);
                if (tileEntity != NullEntity) {
                    if (tileEntity.Has<EC::Inventory>(&game->state->ecs)) {
                        // then open inventory
                        Inventory& inventory = game->state->ecs.Get<EC::Inventory>(tileEntity)->inventory;
                        // for now since I dont want to make GUI so just give the items in the inventory to the player
                        for (Uint32 i = 0; i < inventory.size; i++) {
                            if (game->state->player.inventory()) {
                                Uint32 numItemsAdded = game->state->player.inventory()->addItemStack(inventory[i]);
                                inventory[i].reduceQuantity(numItemsAdded);
                            }
                        }
                    }
                }
            break;}
            case 'o': {
                auto belt = Entities::TransportBelt(&game->state->ecs, Vec2(floor(mouseWorldPos.x), floor(mouseWorldPos.y)));
                // TODO: entity collision stuff
            break;}
            case 'z': {
                auto* heldItemStack = &game->state->player.heldItemStack;
                if (heldItemStack) {
                    ItemStack dropStack = ItemStack(heldItemStack->get()->item, 1);
                    heldItemStack->get()->reduceQuantity(1);
                    Entities::ItemStack(&game->state->ecs, mouseWorldPos, dropStack, game->state->itemManager);
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

        for (int i = 1; i <= (int)game->state->player.numHotbarSlots; i++) {
            if (event.keysym.sym == i + '0') {
                game->state->player.selectHotbarSlot(i - 1);
            }
        }
    }

    switch (keycode) {
        case SDLK_RETURN:
            enteringText = !enteringText;
            game->gui->console.showLog = enteringText;
            game->gui->console.promptOpen = enteringText;
            break;
        case SDLK_SLASH:
            if (!enteringText) {
                enteringText = true;
                game->gui->console.showLog = true;
                game->gui->console.promptOpen = true;
            }
            break;
        default:
            break;
    }
}

void PlayerControls::handleCommandInput(CommandInput commandInput) {
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

        game->gui->console.newMessage(message.c_str(), GUI::Console::MessageType::CommandResult);
    }
}

void PlayerControls::update() {
    if (mouse.leftButtonDown()) {
        leftMouseHeld(mouse);
    }
    if (mouse.rightButtonDown()) {
        rightMouseHeld(mouse);
    }

    if (keyboard.keyState[SDL_SCANCODE_G]) {
        Tile* selectedTile = getTileAtPosition(game->state->chunkmap, mouseWorldPos);
        // TODO: entity collision stuff
        if (selectedTile && false) {
            Entity chest = Entities::Chest(&game->state->ecs, {floor(mouseWorldPos.x), floor(mouseWorldPos.y)}, 32, 1, 1, game->state->itemManager);
            placeEntityOnTile(&game->state->ecs, selectedTile, chest);
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
        SDL_WarpMouseInWindow(game->sdlCtx.win, mouse.x + dx, mouse.y + dy);
    }
}

void PlayerControls::handleEvent(const SDL_Event* event) {
    switch (event->type) {
    case SDL_KEYDOWN:
        handleKeydown(event->key);
        break;
    case SDL_MOUSEBUTTONDOWN:
        handleClick(event->button);
        break;
    case SDL_MOUSEBUTTONUP:
        this->justGrabbedItem = false;
        break;
    case SDL_MOUSEMOTION:
        handleMouseMotion(event->motion);
        break;
    case SDL_TEXTINPUT: {
        const char* text = event->text.text;
        if (enteringText) {
            game->gui->console.enterText(text);
        }
        break;
    }
    default:
        break;
    }
}

void PlayerControls::doPlayerMovementTick() {
    if (enteringText) return; // dont move player while typing... duh

    g.playerMovement = {0.0f, 0.0f};

    int sidewaysInput = keyboard.keyState[SDL_SCANCODE_D] - keyboard.keyState[SDL_SCANCODE_A];
    int updownInput = keyboard.keyState[SDL_SCANCODE_W] - keyboard.keyState[SDL_SCANCODE_S];
    int rotationInput = keyboard.keyState[SDL_SCANCODE_Q] - keyboard.keyState[SDL_SCANCODE_E];

    if (game->state->player.entity.Has<EC::Rotation, EC::ViewBox>(&game->state->ecs)) {
        auto playerRotation = game->state->player.entity.Get<EC::Rotation>(&game->state->ecs);
        if (sidewaysInput || updownInput) {
            glm::vec2 moveVector = glm::normalize(glm::vec2(sidewaysInput, updownInput));
            float angle = glm::radians(playerRotation->degrees);
            glm::vec2 movement = glm::vec2(
                moveVector.x * cos(angle) - moveVector.y * sin(angle),
                moveVector.x * sin(angle) + moveVector.y * cos(angle)
            ) * PLAYER_SPEED;

            movePlayer(movement);
            g.playerMovement = movement;
        }
        
        playerRotation->degrees += rotationInput * PLAYER_ROTATION_SPEED;
    }

    if (controller.connected()) {
        Vec2 leftStick = controller.getLeftStickVector();
        movePlayer(leftStick);
    }
}

void PlayerControls::movePlayer(Vec2 movement) {
    Player* player = &game->state->player;
    auto* oldPlayerPos = game->state->player.get<EC::Dynamic>();
    if (!oldPlayerPos) {
        LogError("no dynamic position component on player!");
        return;
    }
    Vec2 oldPos = oldPlayerPos->pos;
    oldPlayerPos->pos += movement;
}

void PlayerControls::placeItem(ItemStack* item, Vec2 at) {
    Tile* tile = getTileAtPosition(game->state->chunkmap, at);
    if (tile) {
        game->state->player.tryPlaceItemStack(item, tile, game->state->itemManager);
    }
}