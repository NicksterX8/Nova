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

bool canRunConcurrently(
    ComponentAccessType* accessArrayA, ComponentAccessType* accessArrayB,
    bool AContainsStructuralChanges, bool AMustExecuteAfterStructuralChanges,
    bool BContainsStructuralChanges, bool BMustExecuteAfterStructuralChanges
) {
    using namespace ComponentAccess;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const ComponentAccessType accessA = accessArrayA[i];
        const ComponentAccessType accessB = accessArrayB[i];
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

bool canRunConcurrently(EntitySystem* sysA, EntitySystem* sysB) {
    return canRunConcurrently(sysA->sys.componentAccess, sysB->sys.componentAccess,
        sysA->containsStructuralChanges, sysA->mustExecuteAfterStructuralChanges,
        sysB->containsStructuralChanges, sysB->mustExecuteAfterStructuralChanges);
}

void systemJob(EntitySystem* system) {
    // Log("starting job for system %p", system);
    system->Update();
    // Log("finishing job for system %p", system);
}

void runConcurrently(ECS* ecs, EntitySystem* sysA, EntitySystem* sysB) {
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

void playBackCommandBuffer(ECS* ecs, EntityCommandBuffer* buffer) {
    for (auto command : buffer->commands) {
        command(ecs);
    }
    buffer->commands.clear();
}

template<std::size_t NSystems>
void runConcurrently(ECS* ecs, std::array<EntitySystem*, NSystems> systems) {
    constexpr int N = NSystems;

    static_assert(N > 1, "Can not run one or less systems concurrently!");

    ComponentAccessType access[NUM_COMPONENTS] = {0};
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

template<class S>
void runSystem(ECS* ecs) {
    ecs->System<S>()->Update();
    playBackCommandBuffer(ecs, ecs->System<S>()->commandBuffer);
}

static void updateSystems(GameState* state) {
    ECS& ecs = state->ecs;

    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    ecs.System<PositionSystem>()->chunkmap = &state->chunkmap;
    runSystem<PositionSystem>(&ecs);
   
    std::array<EntitySystem*, 3> group = {
        ecs.System<GrowthSystem>(),
        ecs.System<MotionSystem>(),
        ecs.System<RotationSystem>()
    };
    runConcurrently(&ecs, group);

    runSystem<FollowSystem>(&ecs);
    // ecs.System<ExplosivesSystem>()->Update();
    runSystem<ExplosivesSystem>(&ecs);

    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    runSystem<PositionSystem>(&ecs);

    ecs.System<ExplosionSystem>()->Update(&ecs, state->chunkmap);
    playBackCommandBuffer(&ecs, ecs.System<ExplosionSystem>()->commandBuffer);
    runSystem<InserterSystem>(&ecs);
    runSystem<InventorySystem>(&ecs);
    runSystem<HealthSystem>(&ecs);
    runSystem<RotatableSystem>(&ecs);
    runSystem<DyingSystem>(&ecs);

    state->chunkmap.iterateChunkdata([](ChunkData* chunkdata){
        chunkdata->closeEntities.clear();
        return 0;
    });
    runSystem<PositionSystem>(&ecs);
}

int tick(GameState* state) {
    state->player.grenadeThrowCooldown--;
    updateSystems(state);
    return 0;
}

// Main game loop
int update(Context ctx) {
    ctx.metadata.tick();

    bool quit = false;

    int renWidth;
    int renHeight;
    SDL_GetRendererOutputSize(ctx.sdlCtx.ren, &renWidth, &renHeight);
    //*ctx.gameViewport = newGameViewport(renWidth, renHeight, state->player.x, state->player.y);

    GameViewport* gameViewport = ctx.gameViewport;
    GameState* state = ctx.state;

    updateTilePixels(ctx.worldScale);

    //Vec2 focus = state->player.getPosition();
    //*gameViewport = newGameViewport(renWidth, renHeight, focus.x, focus.y);

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    Vec2 playerTargetPos = gameViewport->pixelToWorldPosition(mouse.x, mouse.y);
    // handle events //
    PlayerControls& playerControls = *ctx.playerControls;
    playerControls.updateState();

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch(event.type) {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    SDL_GetRendererOutputSize(ctx.sdlCtx.ren, &renWidth, &renHeight);
                }
                break;
            case SDL_MOUSEWHEEL:
                ctx.worldScale += event.wheel.preciseY / 50.0f;
                if (ctx.worldScale < 0.1f) {
                    ctx.worldScale = 0.1f;
                } else if (ctx.worldScale > 8) {
                    ctx.worldScale = 8;
                }
                updateTilePixels(ctx.worldScale);
                break;
            case SDL_MOUSEBUTTONDOWN:
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                case '7':
                    GameSave::save(state);
                    break;
                case '9':
                    GameSave::load(state);
                    break;
                }
            default:
                break;
        }

        playerControls.handleEvent(event, state, ctx.gui);
    }

    playerControls.update(state, ctx.gui, &ctx);

    if (playerTargetPos != ctx.lastUpdatePlayerTargetPos) {
        playerControls.playerMouseTargetMoved(mouse, ctx.lastUpdateMouseState, state, ctx.gui);
    }

    // these two functions should be combined
    playerControls.doPlayerMovementTick(ctx.state);
    tick(ctx.state);

    // update to new state from tick
    Vec2 focus = state->player.getPosition();
    *gameViewport = newGameViewport(renWidth, renHeight, focus.x, focus.y);
    playerTargetPos = gameViewport->pixelToWorldPosition(mouse.x, mouse.y);

    render(ctx.sdlCtx.ren, ctx.sdlCtx.scale, ctx.gui, state, gameViewport, playerTargetPos);    

    ctx.lastUpdateMouseState = mouse;
    ctx.lastUpdatePlayerTargetPos = playerTargetPos;

    if (quit) {
        Log("Returning from main update loop.");
        return 1;
    }
    return 0; 
}

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param) {
    struct Context *ctx = (struct Context*)param;
    return update(*ctx);
}