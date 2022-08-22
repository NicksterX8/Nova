#include "update.hpp"

#include <thread>
#include <vector>

#include "NC/colors.h"
#include "NC/utils.h"
#include "Log.hpp"
#include "Debug.hpp"
#include "GUI.hpp"
#include "PlayerControls.hpp"
#include "Rendering/rendering.hpp"
#include "GameSave/main.hpp"

#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"
#include "Entities/Entities.hpp"
#include "EntityComponents/Components.hpp"
#include "ECS/ECS.hpp"

Vec2 getMouseWorldPosition(const GameViewport& gameViewport) {
    SDL_Point pixelPosition = SDLGetMousePixelPosition();
    return gameViewport.pixelToWorldPosition(pixelPosition.x, pixelPosition.y);
}

static void updateTilePixels(float scale) {
    if (scale == 0) {
        scale = 1;
    } else if (scale < 0) {
        scale = -scale;
    }
    TilePixels = DEFAULT_TILE_PIXEL_SIZE * SDLPixelScale * scale;
    TileWidth = DEFAULT_TILE_PIXEL_SIZE * SDLPixelScale * scale;
    TileHeight = TileWidth * TILE_PIXEL_VERTICAL_SCALE;
}

GameViewport newGameViewport(int renderWidth, int renderHeight, float focusX, float focusY) {
    SDL_Rect display = {
        0,
        0,
        renderWidth,
        renderHeight
    };
    float worldWidth = ((float)display.w / TileWidth);
    float worldHeight = ((float)display.h / TileHeight);
    SDL_FRect world = {
        focusX - worldWidth/2.0f,
        focusY - worldHeight/2.0f,
        worldWidth,
        worldHeight
    };

    GameViewport newViewport = GameViewport(
        display,
        world
    );
    return newViewport;
}

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos) {
    Tile* tile = getTileAtPosition(chunkmap, mouseWorldPos);
    if (tile && !tile->entity.Exists(ecs)) {
        Vec2 inputPos = {mouseWorldPos.x + 1, mouseWorldPos.y};
        //Tile* inputTile = getTileAtPosition(chunkmap, inputPos);
        Vec2 outputPos = {mouseWorldPos.x - 1, mouseWorldPos.y};
        //Tile* outputTile = getTileAtPosition(chunkmap, outputPos);
        Entity inserter = Entities::Inserter(ecs, mouseWorldPos.vfloor() + Vec2(0.5f, 0.5f), 1, inputPos.floorToIVec(), outputPos.floorToIVec());
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
    GameViewport& gameViewport = ctx.gameViewport;
    PlayerControls& playerControls = *ctx.playerControls;
    DebugClass& debug = *ctx.debug;

    controls->addKeyBinding(new FunctionKeyBinding('y',
    [&ecs, &gameViewport, &state, &playerControls](){
        auto mouse = playerControls.getMouse();
        Entity zombie = Entities::Enemy(
            &ecs,
            gameViewport.pixelToWorldPosition(mouse.x, mouse.y),
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
        new FunctionKeyBinding('c', [&ecs, &gameViewport](){
            int width = 2;
            int height = 1;
            Vec2 position = getMouseWorldPosition(gameViewport).vfloor() + Vec2(width/2.0f, height/2.0f);
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
        new FunctionKeyBinding('i', [&ecs, &gameViewport, &chunkmap](){
            placeInserter(chunkmap, &ecs, getMouseWorldPosition(gameViewport));
        }),
        new FunctionKeyBinding('r', [&gameViewport, &playerControls, &ecs, &chunkmap](){
            //player.findFocusedEntity(getMouseWorldPosition(gameViewport));
            auto focusedEntity = findPlayerFocusedEntity(ecs, chunkmap, getMouseWorldPosition(gameViewport));
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
        Log.Critical("Attempted to run two incompatible systems together concurrently! Running in sequence.");
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

template<std::size_t NSystems>
void runConcurrently(EntityWorld* ecs, std::array<ECS::EntitySystem*, NSystems> systems) {
    constexpr int N = NSystems;

    static_assert(N > 1, "Can not run one or less systems concurrently!");

    ECS::ComponentAccessType access[NUM_COMPONENTS] = {0};
    bool fail = false;
    for (int i = 0; i < N-1; i++) {
        for (int c = 0; c < NUM_COMPONENTS; c++) {
           access[c] |= systems[i]->sys.componentAccess[c];
        }

        if (true) {
            
        } else {
            fail = true;
            break;
        }
    }

#ifdef EMSCRIPTEN
    fail = true;
#endif

    if (fail) {
        Log.Critical("Failed to run group concurrently! Running sequentially.");
        for (int i = 0; i < N; i++) {
            systemJob(systems[i]);
        }

    } else {
        std::thread threads[N]; 
        for (int i = 0; i < N; i++) {
            threads[i] = std::thread(systemJob, systems[i]);
        }
        
        for (int i = 0; i < N; i++) {
            threads[i].join();
        }
    }

    for (int i = 0; i < N; i++) {
        playBackCommandBuffer(ecs, systems[i]->commandBuffer);
    }

    return;
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
    Log.Info("Total number of entities: %u", ecs.EntityCount());
    for (ECS::ComponentID id = 0; id < ecs.NumComponentPools(); id++) {
        const ECS::ComponentPool* pool = ecs.GetPool(id);
        Log.Info("%s || Size: %u", pool->name, pool->size());
        if (pool->size() > ecs.EntityCount()) {
            Log.Error("Size is too large!");
        }
    }
}

int Game::update() {
    metadata.tick();

    bool quit = false;

    int renWidth;
    int renHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &renWidth, &renHeight);

    updateTilePixels(worldScale);

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    Vec2 playerTargetPos = gameViewport.pixelToWorldPosition(mouse.x, mouse.y);
    // handle events //
    playerControls->updateState();

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_GL_GetDrawableSize(sdlCtx.win, &renWidth, &renHeight);
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
    Vec2 focus = state->player.getPosition();
    gameViewport = newGameViewport(renWidth, renHeight, focus.x, focus.y);
    playerTargetPos = gameViewport.pixelToWorldPosition(mouse.x, mouse.y);

    render(*renderContext, sdlCtx.scale, gui, state, &gameViewport, playerTargetPos);    

    lastUpdateMouseState = mouse;
    lastUpdatePlayerTargetPos = playerTargetPos;

    if (quit) {
        Log("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

void Game::init(int screenWidth, int screenHeight) {
    this->state = new GameState();
    this->state->init(NULL, &gameViewport);
    for (int e = 0; e < 20000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        auto tree = Entities::Tree(&state->ecs, pos, {1, 1});
    }

    this->gameViewport = newGameViewport(screenWidth, screenHeight, state->player.getPosition().x, state->player.getPosition().y);
    this->renderContext = new RenderContext(sdlCtx.win, sdlCtx.gl);
    initRenderContext(this->renderContext);
    this->worldScale = 1.0f;
    this->playerControls = new PlayerControls(gameViewport);
    SDL_Point mousePos = SDLGetMousePixelPosition();
    this->lastUpdateMouseState = {
        .x = mousePos.x,
        .y = mousePos.y,
        .buttons = SDLGetMouseButtons()
    };

    setDefaultKeyBindings(*this, playerControls);
}

void Game::destroy() {
    delete this->debug;
    delete this->playerControls;
    delete this->renderContext;
}

void Game::start() {
    // GameSave::load()

    Log.Info("Starting!");

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

void Game::quit() {
    Log.Info("Quitting!");
    double secondsElapsed = metadata.end();
    Log.Info("Time elapsed: %.1f", secondsElapsed);
    // GameSave::save()
}