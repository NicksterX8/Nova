#include "update.hpp"
#include "Entities/Systems/Rendering.hpp"
#include "Entities/Systems/Systems.hpp"
#include "GUI.hpp"
#include "Debug.hpp"
#include "Entities/Entities.hpp"
#include "Log.hpp"
#include "PlayerControls.hpp"
#include "Rendering/rendering.hpp"
#include "NC/colors.h"
#include "NC/utils.h"

void updateTilePixels(float scale) {
    if (scale == 0) {
        scale = 1;
    } else if (scale < 0) {
        scale = -scale;
    }
    TilePixels = DEFAULT_TILE_PIXEL_SIZE * SDLPixelScale * scale;
}

GameViewport newGameViewport(int renderWidth, int renderHeight, float focusX, float focusY) {
    SDL_Rect display = {
        0,
        0,
        renderWidth,
        renderHeight
    };
    float worldWidth = ((float)display.w / TilePixels);
    float worldHeight = ((float)display.h / TilePixels);
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

void updateSystems(GameState* state) {
    ECS& ecs = state->ecs;
    
    ecs.System<RotationSystem>()->Update(&ecs);
    ecs.System<GrowthSystem>()->Update(&ecs);
    ecs.System<FollowSystem>()->Update(&ecs);
    ecs.System<MotionSystem>()->Update(&ecs);
    ecs.System<ExplosivesSystem>()->Update(&ecs);
    ecs.System<ExplosionSystem>()->Update(&ecs);
    ecs.System<InserterSystem>()->Update(&ecs, state->chunkmap);
    ecs.System<InventorySystem>()->Update(&ecs);
    ecs.System<HealthSystem>()->Update(&ecs);
    ecs.System<DyingSystem>()->Update(&ecs);
    ecs.System<RotatableSystem>()->Update(&ecs);
    ecs.DoScheduledRemoves<COMPONENTS>();
}


// Main game loop
int update(Context ctx) {
    Metadata.tick();

    bool quit = false;

    int renWidth;
    int renHeight;
    SDL_GetRendererOutputSize(ctx.sdlCtx.ren, &renWidth, &renHeight);
    //*ctx.gameViewport = newGameViewport(renWidth, renHeight, state->player.x, state->player.y);

    GameViewport* gameViewport = ctx.gameViewport;
    GameState* state = ctx.state;

    updateTilePixels(ctx.worldScale);

    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();
    // handle events //
    PlayerControls playerControls = PlayerControls(gameViewport, mouse, ctx.keyboard); // player related event handler

    int mouseCount = 0;

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
                mouseCount++;
                if (mouseCount > 1) {
                    Log("Mouse clicked more than once in a frame!");
                }
                break;
            default:
                break;
        }

        playerControls.handleEvent(event, state, ctx.gui);
    }

    state->player.grenadeThrowCooldown--;

    playerControls.update(mouse, ctx.keyboard, state, ctx.gui);
    Vec2 focus = state->player.getPosition();
    *gameViewport = newGameViewport(renWidth, renHeight, focus.x, focus.y);

    Vec2 playerTargetPos = gameViewport->pixelToWorldPosition(mouse.x, mouse.y);
    if (playerTargetPos != ctx.lastUpdatePlayerTargetPos) {
        playerControls.playerMouseTargetMoved(mouse, ctx.lastUpdateMouseState, state, ctx.gui);
    }

    int numTilesAccessed = 0;
    
    auto callback = [&numTilesAccessed](ChunkData* chunkdata){
        int count;
        for (int row = 0; row < CHUNKSIZE; row++) {
            for (int col = 0; col < CHUNKSIZE; col++) {
                Tile* tile = &TILE(chunkdata->chunk, row, col);
                //if (tile->type == TileTypes::Space) {
                    //IVec2 tilePos = {chunkdata->tilePositionX() + col + randomInt(-1, 1), chunkdata->tilePositionY() + row + randomInt(-1, 1)};
                    //Tile* adjacentTile = getTileAtPosition(state->chunkmap, tilePos);
                    //if (adjacentTile) {
                    //    adjacentTile->type = TileTypes::Grass;
                    //}

                    tile->type = count + 1;
                    count = (count + 1) % 7;
                //}

                numTilesAccessed++;
            }
        }

        return 0;
    };
    //state->chunkmap.iterateChunkdata(callback);

    //Log("num Tiles accessed: %d", numTilesAccessed);

    //if (!(Metadata.ticks() % 60))
    //    state->player.inventory.addItemStack(ItemStack(0, 1000));

    updateSystems(state);

    /*
    if (!(Metadata.ticks() % 60)) {
        Log("Num entities in explosives system: %u", state->ecs.System<ExplosivesSystem>()->NumEntities());
        Log("Num entities in motion system: %u", state->ecs.System<MotionSystem>()->NumEntities());
        Log("num entities with explosive components: %u", state->ecs.componentPoolSize<ExplosiveComponent>());
        Log("num entities with motion components: %u", state->ecs.componentPoolSize<MotionComponent>());
    }
    */

    render(ctx.sdlCtx.ren, ctx.sdlCtx.scale, ctx.gui, state, gameViewport, playerTargetPos);    

    ctx.lastUpdateMouseState = mouse;
    ctx.lastUpdatePlayerTargetPos = playerTargetPos;

    if (quit) {
        Log("update func sending quit");
        return 1;
    }
    return 0; 
}

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param) {
    struct Context *ctx = (struct Context*)param;
    return update(*ctx);
}