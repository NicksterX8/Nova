#include "Game.hpp"

#include <thread>

#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "utils/random.hpp"
#include "GUI/Gui.hpp"
#include "PlayerControls.hpp"
#include "rendering/rendering.hpp"
#include "GameSave/main.hpp"
#include "commands.hpp"

#include "ECS/ECS.hpp"
#include "ECS/systems/systems.hpp"
#include "ECS/entities/entities.hpp"
#include "ECS/components/components.hpp"

#include "llvm/SmallVector.h"
#include "llvm/ArrayRef.h"

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos) {
    Tile* tile = getTileAtPosition(chunkmap, mouseWorldPos);
    // TODO: entity collision stuff
    if (tile) {
        Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
        Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
        Entity inserter = Entities::Inserter(ecs, Vec2(floor(mouseWorldPos.x), floor(mouseWorldPos.y)) + Vec2(0.5f, 0.5f), 1, vecFloori(inputPos), vecFloor(outputPos));
        placeEntityOnTile(ecs, tile, inserter);
    }
}

void rotateEntity(const ComponentManager<EC::Rotation, EC::Rotatable>& ecs, EntityT<EC::Rotation, EC::Rotatable> entity, bool counterClockwise) {
    float* rotation = &entity.Get<EC::Rotation>(&ecs)->degrees;
    auto rotatable = entity.Get<EC::Rotatable>(&ecs);
    // left shift switches direction
    if (counterClockwise) {
        *rotation -= rotatable->increment;
    } else {
        *rotation += rotatable->increment;
    }
    rotatable->rotated = true;
}

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls) {
    GameState& state = *ctx.state;
    Player& player = state.player;
    EntityWorld& ecs = state.ecs;
    ChunkMap& chunkmap = state.chunkmap;
    Camera& camera = ctx.camera;
    PlayerControls& playerControls = *ctx.playerControls;
    Vec2& mouseWorldPos = playerControls.mouseWorldPos;
    DebugClass& debug = *ctx.debug;
    ItemManager& itemManager = state.itemManager;

    controls->addKeyBinding(new FunctionKeyBinding('y',
    [&ecs, &camera, &state, &playerControls](){
        auto& mouse = playerControls.mouse;
        Entity zombie = Entities::Enemy(
            &ecs,
            camera.pixelToWorld(mouse.x, mouse.y),
            state.player.entity
        );
        (void)zombie;
    }));

    KeyBinding* keyBindings[] = {
        new ToggleKeyBinding<bool>('b', &debug.settings.drawChunkBorders),
        new ToggleKeyBinding<bool>('u', &debug.settings.drawEntityRects),
        new ToggleKeyBinding<bool>('c', &debug.settings.drawChunkCoordinates),
        new ToggleKeyBinding<bool>('[', &debug.settings.drawChunkEntityCount),

        new FunctionKeyBinding('q', [&player](){
            player.releaseHeldItem();
        }),
        
        new FunctionKeyBinding('c', [&itemManager, &ecs, &playerControls](){
            int width = 2;
            int height = 1;
            Vec2 position = vecFloor(playerControls.mouseWorldPos) + Vec2(width/2.0f, height/2.0f);
            auto chest = Entities::Chest(&ecs, position, 3, width, height, itemManager);
            (void)chest;
        }),
        new FunctionKeyBinding('l', [&ecs](){
            // do airstrikes row by row
            for (int y = -100; y < 100; y += 5) {
                for (int x = -100; x < 100; x += 5) {
                    Entities::Airstrike(&ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
                }
            }
        }),
        new FunctionKeyBinding('i', [&ecs, &mouseWorldPos, &chunkmap](){
            placeInserter(chunkmap, &ecs, mouseWorldPos);
        }),
        new FunctionKeyBinding('r', [&playerControls, &ecs, &chunkmap](){
            auto focusedEntity = findPlayerFocusedEntity(ecs, chunkmap, playerControls.mouseWorldPos);
            if (focusedEntity.Has<EC::Rotation, EC::Rotatable>(&ecs))
                rotateEntity(ComponentManager<EC::Rotation, EC::Rotatable, EC::Position>(&ecs), focusedEntity.cast<EC::Rotation, EC::Rotatable>(), playerControls.keyboard.keyState[SDL_SCANCODE_LSHIFT]);
        }),
        // testing stuff
        new FunctionKeyBinding('5', [&](){
            ecs.IterateEntities([&](Entity entity){
                // dont kill player plz
                if (entity.id != player.entity.id) {
                    ecs.Destroy(entity);
                }
            });
        }),
        
    };

    for (size_t i = 0; i < sizeof(keyBindings) / sizeof(KeyBinding*); i++) {
        controls->addKeyBinding(keyBindings[i]);
    }
}

/*
bool canRunConcurrently(
    ECS::ComponentAccessType* accessArrayA, ECS::ComponentAccessType* accessArrayB,
    bool AContainsStructuralChanges, bool AMustExecuteAfterStructuralChanges,
    bool BContainsStructuralChanges, bool BMustExecuteAfterStructuralChanges
) {
    using namespace ECS::ComponentAccess;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const ECS::ComponentAccessType accessA = accessArrayA[i];
        const ECS::ComponentAccessType accessB = accessArrayB[i];
        if (accessA & (Write | Remove)) {
            if (accessB & (Write | Read | Remove | Add | Flag)) {
                // fail
                return false;
            }
        }
        else if (accessB & (Write | Remove)) {
            if (accessA & (Write | Read | Remove | Add | Flag)) {
                // fail
                return false;
            }
        }

        if ((accessA & Add) && (accessB & (Add | Remove | Write | Read | Flag))) {
            return false;
        }

        if ((accessB & Add) && (accessA & (Add | Remove | Write | Read | Flag))) {
            return false;
        }

        if (AContainsStructuralChanges) {
            if (BMustExecuteAfterStructuralChanges) {
                // fail
                return false;
            }
        }

        if (BContainsStructuralChanges) {
            if (AMustExecuteAfterStructuralChanges) {
                // fail
                return false;
            }
        } 
    }

    return true;
}

bool canRunConcurrently(ECS::EntitySystem* sysA, ECS::EntitySystem* sysB) {
    return canRunConcurrently(sysA->sys.componentAccess, sysB->sys.componentAccess,
        sysA->containsStructuralChanges, sysA->mustExecuteAfterStructuralChanges,
        sysB->containsStructuralChanges, sysB->mustExecuteAfterStructuralChanges);
}

void systemJob(ECS::EntitySystem* system) {
    // Log("starting job for system %p", system);
    system->Update();
    // Log("finishing job for system %p", system);
}

void runConcurrently(EntityWorld* ecs, ECS::EntitySystem* sysA, ECS::EntitySystem* sysB) {
    if (!canRunConcurrently(sysA, sysB)) {
        LogCritical("Attempted to run two incompatible systems together concurrently! Running in sequence.");
        systemJob(sysA);
        systemJob(sysB);
        return;
    }

    std::thread thread2(systemJob, sysA);
    std::thread thread3(systemJob, sysB);

    thread2.join();
    thread3.join();

    return;
}

void playBackCommandBuffer(EntityWorld* ecs, ECS::EntityCommandBuffer* buffer) {
    //for (auto command : buffer->commands) {
    //    command(ecs);
    //}
    //buffer->commands.clear();
}
*/
/*
template<class S>
void runSystem(EntityWorld* ecs) {
    ecs->System<S>()->Update();
    playBackCommandBuffer(ecs, ecs->System<S>()->commandBuffer);
}
*/

static void updateSystems(GameState* state) {
    EntityWorld& ecs = state->ecs;

    MotionSystem motionSystem;
    motionSystem.Update(ecs, &state->chunkmap);

    /*
    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    ecs.System<PositionSystem>()->chunkmap = &state->chunkmap;
    runSystem<PositionSystem>(&ecs);
    */

   /*
    ecs.System<MotionSystem>()->m_ecs = &ecs;
    ecs.System<MotionSystem>()->m_chunkmap = &state->chunkmap;

    std::array<EntitySystem*, 3> group = {
        ecs.System<GrowthSystem>(),
        ecs.System<MotionSystem>(),
        ecs.System<RotationSystem>()
    };
    runConcurrently(&ecs, group);

    runSystem<FollowSystem>(&ecs);
    // ecs.System<ExplosivesSystem>()->Update();
    runSystem<ExplosivesSystem>(&ecs);
    */

    /*
    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    runSystem<PositionSystem>(&ecs);
    */

   /*
    ecs.System<ExplosionSystem>()->Update(&ecs, state->chunkmap);
    playBackCommandBuffer(&ecs, ecs.System<ExplosionSystem>()->commandBuffer);
    runSystem<InserterSystem>(&ecs);
    runSystem<InventorySystem>(&ecs);
    runSystem<HealthSystem>(&ecs);
    runSystem<RotatableSystem>(&ecs);
    runSystem<DyingSystem>(&ecs);
    */

    /*
    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    runSystem<PositionSystem>(&ecs);
    */

    ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Position, EC::Fresh> > >(
    [&](Entity entity){
        auto fresh = ecs.Get<EC::Fresh>(entity); assert(fresh);
        if (fresh->flags.getComponent<EC::Position>()) {
            // ec::position just added
            fresh->flags.setComponent<EC::Position>(0);
        }
    });

    ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::Fresh> > >(
    [&](Entity entity){
        ecs.Remove<EC::Fresh>(entity);
    });
}

int tick(GameState* state, PlayerControls* playerControls) {
    state->player.grenadeThrowCooldown--;
    updateSystems(state);
    if (getTick() % 1 == 0) {
        int i = 0;
        /*
        for (auto& chunk: state->chunkmap.chunkList) {
            for (int y = 0; y < CHUNKSIZE; y++) {
                for (int x = 0; x < CHUNKSIZE; x++) {
                    //chunk[y][x].type = TileTypes::Grass;
                    TileType& type = chunk[y][x].type;
                    if (type == TileTypes::Grass) {
                        type = TileTypes::Sand;
                    }
                    else if (type == TileTypes::Sand) {
                        if (randomInt(0, 3) == 0) {
                            type = TileTypes::Grass;
                        }
                    } else {
                        chunk[y][x].type = randomInt(TileTypes::Grass, TileTypes::Wall);
                    }
                }
            }
        }
        */
        /*
        for (int x = -200; x < 200; x++) {
            for (int y = -200; y < 200; y++) {
                #define GETT(x, y) getTileAtPosition(state->chunkmap, IVec2{x, y})
                #define GETTT(x, y) GETT(x, y)->type
                Tile* tile = getTileAtPosition(state->chunkmap, IVec2{x, y});
                if (tile) {
                    TileType& type = tile->type;
                    if (GETT(x, y+1) && GETT(x, y-1) && GETTT(x, y+1) == GETTT(x, y-1)) {
                        type = GETTT(x, y+1);
                    }
                    if (GETT(x+1, y) && GETT(x-1, y) && GETTT(x+1, y) == GETTT(x-1, y)) {
                        GETTT(x+1, y) = type;
                        GETTT(x-1, y) = type;
                    }
                }
            }
        }
        */
    }

    playerControls->doPlayerMovementTick(state);

    return 0;
}

void logComponentPoolSizes(const EntityWorld& ecs) {
    LogInfo("Total number of entities: %u", ecs.EntityCount());
    for (ECS::ComponentID id = 0; id < ecs.NumComponentPools(); id++) {
        const ECS::ComponentPool* pool = ecs.GetPool(id);
        LogInfo("%s || Size: %u", pool->name, pool->size());
        if (pool->size() > ecs.EntityCount()) {
            LogError("Size is too large!");
        }
    }
}

void displaySizeChanged(SDL_Window* window, const RenderContext& renderContext, Camera* camera) {
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);
    glViewport(0, 0, drawableWidth, drawableHeight);
    camera->pixelWidth = (float)drawableWidth;
    camera->pixelHeight = (float)drawableHeight;

    resizeRenderBuffer(renderContext.framebuffer, {drawableWidth, drawableHeight});
}

int Game::handleEvent(const SDL_Event* event) {
    int code = 0;
    switch(event->type) {
        case SDL_QUIT:
            code = 1; 
            break;
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {
                displaySizeChanged(sdlCtx.win, *renderContext, &camera);
            }
            if (event->window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
                float oldPixelScale = SDL::pixelScale;
                SDL::pixelScale = SDL::getPixelScale(sdlCtx.win);
                camera.baseScale = BASE_UNIT_SCALE * SDL::pixelScale;
                float newFontScale = renderContext->font.currentScale / oldPixelScale * SDL::pixelScale;
                float newDebugFontScale = renderContext->debugFont.currentScale / oldPixelScale * SDL::pixelScale;
                renderContext->font.scale(newFontScale);
                renderContext->debugFont.scale(newDebugFontScale);
                displaySizeChanged(sdlCtx.win, *renderContext, &camera);
            }
            break;
        case SDL_MOUSEWHEEL: {
            float wheelMovement = event->wheel.preciseY;
            if (wheelMovement != 0.0f) {
                const float min = 1.0f / 16.0f;
                const float max = 16.0f;
                const float logMin = log2(min);
                const float logMax = log2(max);

                const float wheelMax = 32.0f;

                static float wheelPos = 16.0f;
                wheelPos += wheelMovement;

                if (wheelPos < 1.0f) {
                    wheelPos = 1.0f;
                } else if (wheelPos > wheelMax) {
                    wheelPos = wheelMax;
                }

                float logZoom = ((logMax - logMin) / (wheelMax)) * wheelPos + logMin;
                float zoom = pow(2, logZoom);
                
                camera.zoom = zoom;
            }
        break;}    
        case SDL_KEYDOWN: {
            switch (event->key.keysym.sym) {
                case ']':
                    logComponentPoolSizes(state->ecs);
                    break;
            }
        break;}
        case SDL_CONTROLLERDEVICEADDED: {
            int joystickIndex = event->jdevice.which;
            LogInfo("Controller device added. Joystick index: %d", joystickIndex);
            playerControls->connectController();
        break;}
        case SDL_CONTROLLERDEVICEREMOVED: {
            int instanceID = event->jdevice.which;
            LogInfo("Controller device removed. Instance id: %d", instanceID);
            playerControls->controller.disconnect();
        break;}
        case SDL_CONTROLLERBUTTONDOWN: {
            Uint8 buttonPressed = event->cbutton.button;
            if (buttonPressed == SDL_CONTROLLER_BUTTON_A) {
                playerControls->movePlayer(state, playerControls->mouseWorldPos - state->player.getPosition());
            }
        break;}
        default:
        break;
    }

    playerControls->handleEvent(event, state, gui);
    return code;
}

int Game::update() {
    
    constexpr double targetFPS = 60.0;
    constexpr double fixedFrametime = 1000.0 / targetFPS;
    constexpr int maxUpdates = 3;

    bool quit = false;

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    // handle events //
    playerControls->updateState();

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        int code = handleEvent(&event);
        if (code == 1) quit = true;
    }

    playerControls->update(sdlCtx.win, state, gui);
    if (playerControls->mouseWorldPos != lastUpdatePlayerTargetPos) {
        playerControls->playerMouseTargetMoved(mouse, lastUpdateMouseState, state, gui);
    }

    auto frame = metadata.newFrame();
    double deltaTime = metadata.deltaTime;

    if (deltaTime > 200.0) {
        LogWarn("Slow frame! dt: %f", deltaTime);
    }

    static double remainingTime = 0.0;
    remainingTime += deltaTime;
    
    int updates = 0;
    
    while (remainingTime > fixedFrametime) {
        remainingTime -= fixedFrametime;

        if (updates++ < maxUpdates) {
            Tick currentTick = newTick();
            tick(state, playerControls);
        }
    }

    // update to new state from tick
    Vec2 focus = state->player.getPosition();

    camera.position.x = focus.x;
    camera.position.y = focus.y;
    assert(isValidEntityPosition(camera.position));
    // in full release maybe just set the position to 0,0

    float scale = SDL::pixelScale;
    RenderOptions options = {
        {camera.pixelWidth, camera.pixelHeight},
        scale
    };

    render(*renderContext, options, gui, state, camera, *playerControls, mode, true); 

    lastUpdateMouseState = mouse;
    lastUpdatePlayerTargetPos = playerControls->mouseWorldPos;

    if (quit) {
        LogInfo("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

int Game::init(int screenWidth, int screenHeight) {
    LogInfo("Starting game init");

    this->gui = new Gui();
    this->debug->console = &this->gui->console;

    this->state = new GameState();
    this->state->init();
    loadTileData(this->state->itemManager);
    for (int e = 0; e < 200; e++) {
        Vec2 pos = {(float)randomInt(-400, 400), (float)randomInt(-400, 400)};
        // do placing collision checks
        auto tree = Entities::Tree(&state->ecs, pos, {4, 4});
        (void)tree;
    }

    this->renderContext = new RenderContext(sdlCtx.win, sdlCtx.gl);
    this->playerControls = new PlayerControls(this->camera);
    SDL_Point mousePos = SDL::getMousePixelPosition();
    this->lastUpdateMouseState = {
        .x = mousePos.x,
        .y = mousePos.y,
        .buttons = SDL::getMouseButtons()
    };

    setDefaultKeyBindings(*this, playerControls);

    glViewport(0, 0, screenWidth, screenHeight);
    this->camera = Camera(BASE_UNIT_SCALE, glm::vec3(0.0f), screenWidth, screenHeight);

    LogInfo("starting render init");  
    renderInit(*renderContext, screenWidth, screenHeight);

    setCommands(this);

    //resizeRenderBuffer(renderContext->framebuffer, {screenWidth+1, screenHeight});

    return 0;
}

void Game::quit() {
    LogInfo("Quitting!");
    mode = Quit;

    renderQuit(*renderContext);

    double secondsElapsed = metadata.end();
    LogInfo("Time elapsed: %.1f", secondsElapsed);
    // GameSave::save()
}

void Game::destroy() {
    LogInfo("destroying game");
    delete this->debug;
    delete this->playerControls;
    delete this->renderContext;
}

int Game::start() {
    // GameSave::load()

    Metadata = &metadata;

    LogInfo("Starting!");
    mode = Playing;

    if (metadata.vsyncEnabled) {
        metadata.setTargetFps(60);
    } else {
        metadata.setTargetFps(TARGET_FPS);
    }
    metadata.start();
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(Game::emscriptenUpdateWrapper, this, 0, 1);
#else
    int code;
    do {
        code = update();
    } while (code == 0);
#endif
    return 0;
}