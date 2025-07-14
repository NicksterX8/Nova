#include "PlayerControls.hpp"
#include "Game.hpp"
#include "utils/vectors_and_rects.hpp"

MouseState getMouseState() {
    float mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDL::pixelScale;
    mouseY *= SDL::pixelScale;
    return {
        {mouseX, mouseY},
        buttons
    };
}

bool Controller::connect(int joystick_index) {
    if (connected()) disconnect();

    int numJoysticks = 0;
    SDL_JoystickID* joysticks = SDL_GetJoysticks(&numJoysticks);
    if (numJoysticks) {
        SDL_JoystickID firstJoystick = joysticks[0];
        gameController = SDL_OpenGamepad(firstJoystick);
        joystickIndex = 0;
        if (gameController) {
            type = SDL_GetGamepadType(gameController);
            controllerName = TypeNames[type];
            LogInfo("Connected %s controller with joystick index %d.", controllerName, joystickIndex);
        } else {
            LogError("Failed to open game controller - Error: %s", SDL_GetError());
        }
    }
    SDL_free(joysticks);
    return false;
}

PlayerControls::PlayerControls(Game* game) : camera(game->camera), game(game) {
    mouse = getMouseState();
    mouseClicked = false;
    mouseWorldPos = camera.pixelToWorld(mouse.position);

    connectController();
    
    enteringText = false;
    justGrabbedItem = false;
}

bool PlayerControls::pixelInWorld(glm::vec2 pixel) {
    bool clickInDisplay = pointInRect(pixel, camera.displayViewport);
    bool clickOnGui = game->gui.pointInArea(pixel);
    return clickInDisplay && !clickOnGui;
}

std::vector<GameAction> PlayerControls::handleClick(const SDL_MouseButtonEvent& event) {
    mouseClicked = true;

    std::vector<GameAction> actions;

    glm::vec2 mousePos = mousePixelPos();

    bool mouseClickedOnNewGui = false;

    GUI::Element hoveredGuiElement = game->gui.manager.hoveredElement;
    if (hoveredGuiElement != GUI::NullElement) {
        mouseClickedOnNewGui = true;
        if (auto* button = game->gui.manager.getComponent<GUI::EC::Button>(hoveredGuiElement)) {
            for (auto& action : button->onClick) {
                actions.push_back(guiActionToGameAction(action, hoveredGuiElement));
            }
        }
    }

    // mouse event coordinates are reported in pixel points, which are not representative of actual pixels
    // on high DPI displays, so we scale it by the pixel scale to get actual pixels.
    //SDL_Point mousePos = {(int)(event.x * SDL::pixelScale), (int)(event.y * SDL::pixelScale)};
    //Vec2 worldPos = camera.pixelToWorld(mousePos.x, mousePos.y);
    bool clickInDisplay = pointInRect(mousePos, camera.displayViewport);
    bool clickOnGui = game->gui.pointInArea(mousePos) || mouseClickedOnNewGui;
    bool clickInWorld = clickInDisplay && !clickOnGui;
    Tile* selectedTile = getTileAtPosition(game->state->chunkmap, mouseWorldPos);
    if (event.button == SDL_BUTTON_LEFT) {

        // make sure mouse is within display viewport
        if (clickInDisplay) {
            // first check for GUI clicks
            if (clickOnGui) {
                // click counted as on gui, not world.
                // trigger gui stuff
               
            } else {
                // World click
                ItemStack* heldItemStack = game->state->player.heldItemStack.get();
                auto& itemMgr = game->state->itemManager;
                if (game->state->player.isHoldingItem()) {
                    if (itemMgr.entityHas<ITC::Placeable>(heldItemStack->item)) {
                        placeItem(heldItemStack, mouseWorldPos);
                    }
                    if (itemMgr.entityHas<ITC::Usable>(heldItemStack->item)) {
                        auto usable = itemMgr.getComponent<ITC::Usable>(heldItemStack->item);
                        auto onUse = usable->onUse;
                        if (onUse) {
                            bool success = onUse(game);
                            if (success) {
                                if (usable->oneTimeUse)
                                    heldItemStack->reduceQuantity(1);
                            }
                        }
                    }
                }

                // find entity to select
                auto entityOnMouse = findPlayerFocusedEntity(*game->state->ecs, game->state->chunkmap, mouseWorldPos);
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
        }
    }

    return actions;
}

void PlayerControls::leftMouseHeld(const MouseState& mouse) {
    Vec2 mousePos = mouse.position;
    if (pointInRect(mousePos, camera.displayViewport)) {
        glm::vec2 relMousePos = {mousePos.x - camera.displayViewport.x, mousePos.y - camera.displayViewport.y};
        if (!game->gui.pointInArea(relMousePos)) {
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
    namespace EC = World::EC;
    auto& ecs = *game->state->ecs;

    Entity* selectedEntity = &game->state->player.selectedEntity;
    // if the entity is already selected, deselect it, otherwise select the new one
    if (clickedEntity == *selectedEntity) {
        ecs.Remove<EC::Selected>(clickedEntity);
        *selectedEntity = NullEntity;
    } else {
        if (ecs.EntityHas<EC::Selected>(*selectedEntity)) {
            ecs.Remove<EC::Selected>(*selectedEntity);
        }
        ecs.Add<EC::Selected>(clickedEntity, {});
        *selectedEntity = clickedEntity;
    }
    
    auto e = *selectedEntity;
    auto* prototype = ecs.getPrototype(e);
    if (prototype) {
        const char* name = prototype->name.c_str();
        LogInfo("name: %s | id: %d", name, e.id);
    }
    
    auto* box = ecs.Get<EC::CollisionBox>(e);
    auto* position = ecs.Get<EC::Position>(e);
    if (box && position) {
        LogInfo("position: %.1f,%.1f", position->x, position->y);
        LogInfo("size: %.1f,%.1f", box->box.size.x, box->box.size.y);
    }

    if (ecs.EntityHas<EC::Grabbable, EC::ItemStack>(*selectedEntity)) {
        ItemStack itemGrabbed = ecs.Get<EC::ItemStack>(*selectedEntity)->item;
        game->state->player.inventory()->addItemStack(itemGrabbed);
        ecs.Destroy(*selectedEntity);
    }
}

void PlayerControls::playerMouseTargetMoved(const MouseState& mouseState, const MouseState& prevMouseState) {
    Vec2 newWorldPos = camera.pixelToWorld(mouseState.position);
    Vec2 prevWorldPos = camera.pixelToWorld(prevMouseState.position);

    if (mouseState.buttons & SDL_BUTTON_LMASK) {
        // only do it if the isnt in the gui either currently or previously
        if (!game->gui.pointInArea(mouseState.position)) {
            auto line = raytraceDDA(prevWorldPos, newWorldPos);
            StackAllocate<Tile, 8> tiles{line.size()};
            getTiles(game->state->chunkmap, line.data(), tiles.data(), line.size());
            ItemStack* heldItemStack = game->state->player.heldItemStack.get();
            
            if (!justGrabbedItem) {
                if (heldItemStack && game->state->player.canPlaceItemStack(*heldItemStack, game->state->itemManager)) {
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
    const SDL_Keycode keycode = event.key;
    const SDL_Scancode scancode = event.scancode;

    auto commandInput = game->gui.console.handleKeypress(keycode, gCommands, enteringText);
    if (commandInput)
        handleCommandInput(commandInput);

    switch (keycode) {
        case SDLK_RETURN:
            enteringText = !enteringText;
            if (enteringText) {
                SDL_StartTextInput(game->sdlCtx.primary.window);
            } else {
                SDL_StopTextInput(game->sdlCtx.primary.window);
            }
            break;
        case SDLK_SLASH:
            if (!enteringText) {
                SDL_StartTextInput(game->sdlCtx.primary.window);
                enteringText = true;
            }
            break;
        default:
            break;
    }
   
    switch (keycode) {
        case 'z': {
            auto* heldItemStack = &game->state->player.heldItemStack;
            if (heldItemStack && heldItemStack->get()) {
                ItemStack dropStack = ItemStack(heldItemStack->get()->item, 1);
                heldItemStack->get()->reduceQuantity(1);
                World::Entities::ItemStack::make(game->state->ecs, mouseWorldPos, dropStack, game->state->itemManager)();
            }
        break;} 
        case 'h': {
            
        break;}
    }

    if (!enteringText) {
        for (int i = 1; i <= (int)game->state->player.numHotbarSlots; i++) {
            if (event.key == i + '0') {
                game->state->player.selectHotbarSlot(i - 1);
            }
        }
    }
    
}

void PlayerControls::handleCommandInput(CommandInput commandInput) {
    if (!commandInput.name.empty()) {
        // command entered
        std::string message;
        bool foundCommand = false;
        for (auto& command : gCommands) {
            if (My::streq_case(command.name, commandInput.name.c_str(), command.nameLength, commandInput.name.size())) {
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

        game->gui.console.newMessage(message.c_str(), GUI::Console::MessageType::CommandResult);
    }
}

void PlayerControls::update() {
    if (mouse.leftButtonDown()) {
        leftMouseHeld(mouse);
    }
    if (mouse.rightButtonDown()) {
        rightMouseHeld(mouse);
    }
    
    if (!enteringText) {
        for (auto keyBinding : keyboard.bindings) {
            keyBinding->updateKeyState(keyboard.keyState[SDL_GetScancodeFromKey(keyBinding->key, nullptr)]);
        }
    }

    Vec2 rightStick = controller.getRightStickVector();
    if (rightStick.x != 0.0f || rightStick.y != 0.0f) {
        Vec2 delta = rightStick * 12.0f;
        Vec2 mousePos = mouse.position + delta;
        SDL_WarpMouseInWindow(game->sdlCtx.primary.window, mousePos.x, mousePos.y);
    }
}

std::vector<GameAction> PlayerControls::handleEvent(const SDL_Event* event) {
    std::vector<GameAction> actions;
    switch (event->type) {
    case SDL_EVENT_KEY_DOWN:
        handleKeydown(event->key);
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        actions = handleClick(event->button);
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        this->justGrabbedItem = false;
        break;
    case SDL_EVENT_MOUSE_MOTION:
        handleMouseMotion(event->motion);
        break;
    case SDL_EVENT_TEXT_INPUT: {
        const char* text = event->text.text;
        if (enteringText) {
            game->gui.console.enterText(text);
        }
        break;
    }
    default:
        break;
    }

    return actions;
}

void PlayerControls::doPlayerMovementTick() {
    if (enteringText) return; // dont move player while typing... duh

    int sidewaysInput = keyboard.keyState[SDL_SCANCODE_D] - keyboard.keyState[SDL_SCANCODE_A];
    int updownInput = keyboard.keyState[SDL_SCANCODE_W] - keyboard.keyState[SDL_SCANCODE_S];
    int rotationInput = keyboard.keyState[SDL_SCANCODE_Q] - keyboard.keyState[SDL_SCANCODE_E];

    Entity playerEntity = game->state->player.entity;
    if (game->state->ecs->EntityHas<World::EC::Rotation, World::EC::ViewBox>(playerEntity)) {
        auto playerRotation = game->state->ecs->Get<World::EC::Rotation>(playerEntity);
        if (sidewaysInput || updownInput) {
            glm::vec2 moveVector = glm::normalize(glm::vec2(sidewaysInput, updownInput));
            float angle = glm::radians(playerRotation->degrees);
            glm::vec2 movement = glm::vec2(
                moveVector.x * cos(angle) - moveVector.y * sin(angle),
                moveVector.x * sin(angle) + moveVector.y * cos(angle)
            ) * game->state->player.getSpeed();

            movePlayer(movement);
        }
        
        playerRotation->degrees += rotationInput * PLAYER_ROTATION_SPEED;
    }

    if (controller.connected()) {
        Vec2 leftStick = controller.getLeftStickVector();
        movePlayer(leftStick);
    }
}

bool circleCollidingRect(Vec2 circleCenter, float circleRadius, Box rect, Vec2* delta = nullptr) {
    Vec2 halfExtents = rect.size / 2.0f;
    Vec2 rectCenter = rect.min + halfExtents;
    Vec2 difference = circleCenter - rectCenter;
    Vec2 clamped = glm::clamp(difference, -halfExtents, halfExtents);
    Vec2 closest = rectCenter + clamped; // closest point on rect
    difference = closest - circleCenter;
    if (delta) {
        *delta = difference;
    }
    return glm::length(difference) < circleRadius;
}

namespace Direction {
    enum {
        UP,
        RIGHT,
        DOWN,
        LEFT,
        Vertical = 1
    };
}

int VectorDirection(glm::vec2 target)
{
    constexpr glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),	// up
        glm::vec2(1.0f, 0.0f),	// right
        glm::vec2(0.0f, -1.0f),	// down
        glm::vec2(-1.0f, 0.0f)	// left
    };
    float max = 0.0f;
    unsigned int best_match = -1;
    Vec2 norm = glm::normalize(target);
    for (unsigned int i = 0; i < 4; i++)
    {
        float dot_product = glm::dot(norm, compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (int)best_match;
}

Vec2 circleCollidingRect(Vec2 circleCenter, float circleRadius, Box rect, Vec2 movement) {
    Vec2 halfExtents = rect.size / 2.0f;
    Vec2 rectCenter = rect.min + halfExtents;
    Vec2 difference = circleCenter - rectCenter;
    Vec2 clamped = glm::clamp(difference, -halfExtents, halfExtents);
    Vec2 closest = rectCenter + clamped; // closest point on rect



    difference = closest - circleCenter;
    Vec2 adjust = {0,0};
    float distance = glm::length(difference);

    if (distance < circleRadius) {
        auto direction = VectorDirection(difference);
        Vec2 penetration = circleRadius - Vec2{abs(difference.x), abs(difference.y)};
        if (direction == Direction::LEFT || direction == Direction::RIGHT) {
            adjust.x += penetration.x * (Direction::LEFT  == direction);
            adjust.x -= penetration.x * (Direction::RIGHT == direction);
        } else {
            adjust.y -= penetration.y * (Direction::UP   == direction);
            adjust.y += penetration.y * (Direction::DOWN == direction);
        }
    }
    // Vec2 collisionMargin = difference -  * distance;
    // if (distance < circleRadius) {
    //     if (movement.x > 0.0f) {
    //         adjust.x = 
    //     }
    // }w
    return adjust;
}

void PlayerControls::movePlayer(Vec2 movement) {
    Player* player = &game->state->player;
    auto* oldPlayerPos = game->state->player.get<World::EC::Dynamic>();
    if (!oldPlayerPos) {
        LogError("no dynamic position component on player!");
        return;
    }

    float collisionRadius = PLAYER_DIAMETER / 2.0f;

    Vec2 oldPos = oldPlayerPos->pos;
    Vec2 newPos = oldPos + movement;

    Vec2 circleCenter = newPos + collisionRadius;
    Vec2 potentialPosition = newPos + collisionRadius;

    Vec2 min = glm::min(oldPos, newPos);
    Vec2 max = glm::max(oldPos, newPos);
    
    max += collisionRadius*2.0f;

    IVec2 minTile = vecFloori(min);
    IVec2 maxTile = vecCeili(max);

    Vec2 change = {0, 0};
    for (int x = minTile.x; x <= maxTile.x; x++) {
        for (int y = minTile.y; y <= maxTile.y; y++) {
            Tile* tile = getTileAtPosition(game->state->chunkmap, Vec2{x, y});
            if (TileTypeData[tile->type].flags & TileTypes::Solid) {
                // Vec2 tileCenter = Vec2{x + 0.5f, y + 0.5f};
                // Vec2 delta = tileCenter - circleCenter;
                // Vec2 clamped = glm::clamp(delta, {-0.5f, -0.5f}, {0.5f, 0.5f});
                // Vec2 closest = tileCenter + clamped; // closest point on tile
                // delta = closest - circleCenter;
                // bool collision = glm::length(delta) < collisionRadius;
                // if (collision) {
                //     float angle = atan2(delta.y, delta.x);
                //     Vec2 sincos = get_sincosf(angle);
                //     change = sincos * delta;
                //     tile->type = TileTypes::Sand;
                // }
                // Vec2 delta;
                // if (circleCollidingRect(circleCenter, collisionRadius, {Vec2{x, y}, Vec2({1, 1})}, &delta)) {
                //     LogInfo("colliding");
                //     float angle = atan2(delta.y, delta.x);
                //     Vec2 sincos = get_sincosf(angle);
                //     if (movement.x > 0.0f) {
                        
                //     }
                //     change = -(delta + collisionRadius);

                //     change = circleCollidingRect(circleCenter, collisionRadius, {Vec2{x, y}, Vec2({1, 1})}, movement);
                //     goto end;
                // }

                Vec2 nearestPoint = glm::clamp(potentialPosition, {x, y}, {x+1, y+1});
                Vec2 nearestPointDelta = nearestPoint - potentialPosition;
                float length = glm::length(nearestPointDelta);
                float overlap = collisionRadius - length;
                if (overlap > 0.0f) {
                    Vec2 norm;
                    if (length == 0.0f) {
                        norm = {0, 1};
                    } else {
                        norm = glm::normalize(nearestPointDelta);
                    }
                    potentialPosition -= norm * overlap;
                }
            }
        }
    }

    oldPlayerPos->pos = potentialPosition - collisionRadius;
}

void PlayerControls::placeItem(ItemStack* item, Vec2 at) {
    Tile* tile = getTileAtPosition(game->state->chunkmap, at);
    if (tile) {
        game->state->player.tryPlaceItemStack(item, tile, game->state->itemManager);
    }
}