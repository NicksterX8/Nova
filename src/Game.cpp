#include "Game.hpp"

#include <thread>
#include <vector>

#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "GUI/GUI.hpp"
#include "PlayerControls.hpp"
#include "Rendering/rendering.hpp"
#include "GameSave/main.hpp"

#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"
#include "Entities/Entities.hpp"
#include "EntityComponents/Components.hpp"
#include "ECS/ECS.hpp"

Vec2 getMouseWorldPosition(const Camera& camera) {
    SDL_Point pixelPosition = SDL::getMousePixelPosition();
    return camera.pixelToWorld(pixelPosition.x, pixelPosition.y);
}

static void updateTilePixels(float scale) {
    if (scale == 0) {
        scale = 1;
    } else if (scale < 0) {
        scale = -scale;
    }
    TilePixels = DEFAULT_TILE_PIXEL_SIZE * SDL::pixelScale * scale;
    TileWidth = DEFAULT_TILE_PIXEL_SIZE * SDL::pixelScale * scale;
    TileHeight = TileWidth * TILE_PIXEL_VERTICAL_SCALE;
}

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos) {
    Tile* tile = getTileAtPosition(chunkmap, mouseWorldPos);
    if (tile && !tile->entity.Exists(ecs)) {
        Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
        //Tile* inputTile = getTileAtPosition(chunkmap, inputPos);
        Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
        //Tile* outputTile = getTileAtPosition(chunkmap, outputPos);
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

    controls->addKeyBinding(new FunctionKeyBinding('y',
    [&ecs, &camera, &state, &playerControls](){
        auto mouse = playerControls.getMouse();
        Entity zombie = Entities::Enemy(
            &ecs,
            camera.pixelToWorld(mouse.x, mouse.y),
            state.player.entity
        );
    }));

    KeyBinding* keyBindings[] = {
        new ToggleKeyBinding('b', &debug.settings.drawChunkBorders),
        new ToggleKeyBinding('u', &debug.settings.drawEntityRects),
        new ToggleKeyBinding('c', &debug.settings.drawChunkCoordinates),
        new ToggleKeyBinding('[', &debug.settings.drawChunkEntityCount),

        new FunctionKeyBinding('q', [&player](){
            player.releaseHeldItem();
        }),
        new FunctionKeyBinding('c', [&ecs, &camera, &playerControls](){
            int width = 2;
            int height = 1;
            Vec2 position = vecFloor(playerControls.mouseWorldPos) + Vec2(width/2.0f, height/2.0f);
            auto chest = Entities::Chest(&ecs, position, 3, width, height);

        }),
        new FunctionKeyBinding('l', [&ecs](){
            // do airstrikes row by row
            for (int y = -100; y < 100; y += 5) {
                for (int x = -100; x < 100; x += 5) {
                    auto airstrike = Entities::Airstrike(&ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
                }
            }
        }),
        new FunctionKeyBinding('i', [&ecs, &mouseWorldPos, &chunkmap](){
            placeInserter(chunkmap, &ecs, mouseWorldPos);
        }),
        new FunctionKeyBinding('r', [&camera, &playerControls, &ecs, &chunkmap](){
            auto focusedEntity = findPlayerFocusedEntity(ecs, chunkmap, playerControls.mouseWorldPos);
            if (focusedEntity.Has<EC::Rotation, EC::Rotatable>(&ecs))
                rotateEntity(ComponentManager<EC::Rotation, EC::Rotatable, EC::Position>(&ecs), focusedEntity.cast<EC::Rotation, EC::Rotatable>(), playerControls.keyboardState[SDL_SCANCODE_LSHIFT]);
        }),
        new FunctionKeyBinding('5', [&](){
            const Entity* entities = ecs.GetEntityList();
            for (int i = ecs.EntityCount()-1; i >= 0; i--) {
                if (entities[i].id != 0)
                    ecs.Destroy(entities[i]);
            }
        })

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
}

int tick(GameState* state) {
    state->player.grenadeThrowCooldown--;
    updateSystems(state);
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

int Game::update() {
    metadata.tick();

    bool quit = false;

    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &drawableWidth, &drawableHeight);

    updateTilePixels(worldScale);

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    Vec2 playerTargetPos = camera.pixelToWorld(mouse.x, mouse.y);
    // handle events //
    playerControls->updateState();

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        static bool enteringText = false;
        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_GL_GetDrawableSize(sdlCtx.win, &drawableWidth, &drawableHeight);
                    glViewport(0, 0, drawableWidth, drawableHeight);
                    camera.pixelWidth = drawableWidth;
                    camera.pixelHeight = drawableHeight;
                }
                break;
            case SDL_MOUSEWHEEL:
                worldScale += event.wheel.preciseY / 50.0f;
                if (worldScale < 0.1f) {
                    worldScale = 0.1f;
                } else if (worldScale > 8) {
                    worldScale = 8;
                }
                updateTilePixels(worldScale);
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                
                case '7':
                    GameSave::save(state);
                    break;
                case '9':
                    GameSave::load(state);
                    break;
                case ']':
                    logComponentPoolSizes(state->ecs);
                    break;
                }
            default:
                break;
        }

        playerControls->handleEvent(event, state, gui);
    }

    playerControls->update(state, gui);
    if (playerTargetPos != lastUpdatePlayerTargetPos) {
        playerControls->playerMouseTargetMoved(mouse, lastUpdateMouseState, state, gui);
    }

    // these two functions should be combined
    playerControls->doPlayerMovementTick(state);
    tick(state);

    // update to new state from tick
    playerTargetPos = camera.pixelToWorld(mouse.x, mouse.y);
    Vec2 focus = state->player.getPosition();
    camera.position.x = focus.x;
    camera.position.y = focus.y;
    camera.baseScale = TilePixels;
    camera.zoom = 1.0f;

    float scale = SDL::pixelScale;

    render(*renderContext, scale, gui, state, camera, playerTargetPos);    

    lastUpdateMouseState = mouse;
    lastUpdatePlayerTargetPos = playerTargetPos;

    if (quit) {
        LogInfo("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

void Game::init(int screenWidth, int screenHeight) {

    this->gui = new GUI();

    this->state = new GameState();
    this->state->init();
    for (int e = 0; e < 2000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        auto tree = Entities::Tree(&state->ecs, pos, {1, 1});
    }

    this->renderContext = new RenderContext(sdlCtx.win, sdlCtx.gl);
    setTextureMetadata();
    this->worldScale = 1.0f;
    this->playerControls = new PlayerControls(this->camera);
    SDL_Point mousePos = SDL::getMousePixelPosition();
    this->lastUpdateMouseState = {
        .x = mousePos.x,
        .y = mousePos.y,
        .buttons = SDL::getMouseButtons()
    };

    setDefaultKeyBindings(*this, playerControls);

    int displayWidth,displayHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &displayWidth, &displayHeight);
    glViewport(0, 0, displayWidth, displayHeight);
    this->camera = Camera(TilePixels, glm::vec3(0.0f), displayWidth, displayHeight);

    renderInit(*renderContext);
}

void Game::quit() {
    LogInfo("Quitting!");

    renderQuit(*renderContext);

    double secondsElapsed = metadata.end();
    LogInfo("Time elapsed: %.1f", secondsElapsed);
    // GameSave::save()
}

void Game::destroy() {
    delete this->debug;
    delete this->playerControls;
    delete this->renderContext;
}

void Game::start() {
    // GameSave::load()

    Metadata = &metadata;

    LogInfo("Starting!");

    if (metadata.vsyncEnabled) {
        metadata.setTargetFps(60);
    } else {
        metadata.setTargetFps(TARGET_FPS);
    }
    metadata.start();
#ifdef EMSCRIPTEN
    emscripten_set_main_loop_arg(emscriptenUpdateWrapper, this, 0, 1);
#else
    int code;
    do {
        code = update();
    } while (code == 0);
#endif
    this->quit();
}