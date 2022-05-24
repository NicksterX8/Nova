#ifndef UPDATE_INCLUDED
#define UPDATE_INCLUDED

#include <vector>
#include <SDL2/SDL.h>

#include "SDL_FontCache/SDL_FontCache.h"
#include "NC/SDLContext.h"
#include "NC/SDLBuild.h"
#include "NC/colors.h"
#include "NC/utils.h"

#include "rendering.hpp"
#include "constants.hpp"
#include "Textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GameViewport.hpp"
#include "GUI.hpp"
#include "Debug.hpp"
#include "Entity.hpp"
#include "Log.hpp"

struct Context {
    SDLContext& sdlCtx;
    const Uint8 *keyboard;
    GameState* state;
    GUI* gui;
    GameViewport* gameViewport;
    float& worldScale;
    MouseState& lastUpdateMouseState;
    Vec2& lastUpdatePlayerTargetPos;

    Context(SDLContext& sdlContext, GameState* state, GUI* gui, GameViewport* gameViewport,
    float& worldScale, MouseState& lastUpdateMouseState, Vec2& lastUpdatePlayerTargetPos):
    sdlCtx(sdlContext), state(state), gui(gui), gameViewport(gameViewport), worldScale(worldScale),
    lastUpdateMouseState(lastUpdateMouseState), lastUpdatePlayerTargetPos(lastUpdatePlayerTargetPos) {
        keyboard = SDL_GetKeyboardState(NULL);
    }
};

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

    // handle events //
    PlayerControls playerControls = PlayerControls(gameViewport); // player related event handler
 
    // get user input state for this update
    SDL_PumpEvents();
    MouseState mouse = getMouseState();

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
            default:
                break;
        }

        playerControls.handleEvent(event, state, ctx.gui);
    }

    state->player.grenadeThrowCooldown--;

    playerControls.update(mouse, ctx.keyboard, state, ctx.gui);
    *gameViewport = newGameViewport(renWidth, renHeight, state->player.x, state->player.y);

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

    ECS* ecs = &state->ecs;
    state->ecs.iterateComponents<GrowthComponent>(
    [](GrowthComponent* growth){
        growth->growthValue += 0.1;
    });

    state->ecs.iterateEntities<PositionComponent, MotionComponent, ExplosiveComponent>(
    [](ECS* ecs, Entity entity){
        PositionComponent* position = ecs->Get<PositionComponent>(entity);
        Vec2 target = ecs->Get<MotionComponent>(entity)->target;
        Vec2 delta = {target.x - position->x, target.y - position->y};
        Vec2 unit = delta.norm().scaled(ecs->Get<MotionComponent>(entity)->speed);

        if (delta.length() < unit.length()) {
            position->x = target.x;
            position->y = target.y;

            // EXPLODE
            Entity explosion = ecs->New();
            ecs->Add<ExplosionComponent>(explosion, *ecs->Get<ExplosiveComponent>(entity)->explosion);
            ecs->Add<PositionComponent>(explosion, PositionComponent(target));

            ecs->Remove(entity);
        } else {
            position->x += unit.x;
            position->y += unit.y;
        }
    });

    state->ecs.iterateEntities(
    [](Uint32 signature){
        Uint32 need = componentSignature<PositionComponent, MotionComponent>();
        Uint32 avoid = componentSignature<ExplosiveComponent>();
        if (((signature & need) == need) && !(signature & avoid)) {
            return true;
        }
        return false;
    },
    [](ECS* ecs, Entity entity){
        PositionComponent* position = ecs->Get<PositionComponent>(entity);
        Vec2 target = ecs->Get<MotionComponent>(entity)->target;
        Vec2 delta = {target.x - position->x, target.y - position->y};
        Vec2 unit = delta.norm().scaled(1);

        if (delta.length() < unit.length()) {
            position->x = target.x;
            position->y = target.y;

            ecs->RemoveComponents<MotionComponent>(entity);
        } else {
            position->x += unit.x;
            position->y += unit.y;
        }
    });

    ecs->iterateEntities<ExplosionComponent, PositionComponent>(
    [](ECS* ecs, Entity entity){
        ExplosionComponent* explosion = ecs->Get<ExplosionComponent>(entity);
        float radius = explosion->radius;
        //PositionComponent* positionComponent = ecs->Get<PositionComponent>(entity);
        //Vec2 position = {positionComponent->x, positionComponent->y};
        Vec2 position = *ecs->Get<PositionComponent>(entity);
        
        ecs->iterateEntities<PositionComponent, HealthComponent>([position, radius, entity, explosion](ECS* ecs, Entity affectedEntity){
            Vec2 aPos = *ecs->Get<PositionComponent>(affectedEntity);
            Vec2 delta = aPos - position;
            float distanceSqrd = delta.x * delta.x + delta.y * delta.y;
            
            if (distanceSqrd < radius*radius) {
                // entity is in range of explosion,
                // reduce their health
                ecs->Get<HealthComponent>(affectedEntity)->healthValue -= explosion->damage;
            }
        });
        
        explosion->life--;
        if (explosion->life < 1) {
            ecs->Remove(entity);
        }
    });


    int code = ecs->DoScheduledRemoves();
    if (code) {
        LogError("Failed to do scheduled removes. code: %d", code);
    }
    
    state->ecs.iterateEntities<HealthComponent>(
    [](ECS* ecs, Entity entity){
        HealthComponent* health = ecs->Get<HealthComponent>(entity);
        if (health->healthValue <= 0.0f) {
            ecs->Remove(entity);
        }
    });

    //Log("num Tiles accessed: %d", numTilesAccessed);

    //if (!(Metadata.ticks() % 60))
    //    state->player.inventory.addItemStack(ItemStack(0, 1000));

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

#endif