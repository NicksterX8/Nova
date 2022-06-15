#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <vector>

#include <SDL2/SDL.h>
#include "../GameViewport.hpp"
#include "../GameState.hpp"
#include "../Tiles.hpp"
#include "../Metadata.hpp"
#include "../Textures.hpp"
#include "../Debug.hpp"
#include "../GUI.hpp"
#include "../NC/colors.h"
#include "../Log.hpp"

#include "Drawing.hpp"
#include "../EntitySystems/Rendering.hpp"

SDL_Texture* screenTexture = NULL;

int drawWorld(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, GameState* state) {
    int renWidth,renHeight;
    SDL_GetRendererOutputSize(ren, &renWidth, &renHeight);
    if (renWidth != gameViewport->display.w) {
        //Log("width is incorrect! renderwidth: %d, viewport width: %d", renWidth, gameViewport->display.w);
    }

    // draw space background
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderFillRect(ren, NULL);
    // stars

    Vec2 minChunkRelativePos = {gameViewport->world.x / CHUNKSIZE, gameViewport->world.y / CHUNKSIZE};
    Vec2 maxChunkRelativePos = {gameViewport->worldRightEdge() / CHUNKSIZE, gameViewport->worldBottomEdge() / CHUNKSIZE};
    IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);
    
    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
            const Chunk* chunk = state->chunkmap.getChunk({x, y});
            if (!chunk) {
                ChunkData* newChunk = state->chunkmap.createChunk({x, y});
                generateChunk(newChunk->chunk);
                chunk = newChunk->chunk;
            }

            Vec2 screenDst = gameViewport->worldToPixelPositionF(Vec2(x * CHUNKSIZE, y * CHUNKSIZE));
            SDL_FRect destination = {
                screenDst.x,
                screenDst.y,
                TileWidth * CHUNKSIZE,
                TileHeight * CHUNKSIZE
            };
            
            // When zoomed out far, simplify rendering for better performance and less distraction
            if (TileWidth < 10.0f) {
                Draw::simpleChunk(chunk, ren, destination);
            } else {
                Draw::chunkExp(chunk, ren, scale, destination);
                // draw chunk coordinates
                if (Debug.settings.drawChunkCoordinates) {
                    FC_DrawScale(FreeSans, ren, destination.x + 3*scale, destination.y + 2*scale, FC_MakeScale(0.5f,0.5f),
                    "%d, %d", x * CHUNKSIZE, y * CHUNKSIZE);
                }    
            }

        }
    }

    auto renderPositionSystem = state->ecs.System<RenderPositionSystem>();
    renderPositionSystem->gameViewport = gameViewport;
    renderPositionSystem->Update();

    auto renderSizeSystem = state->ecs.System<RenderSizeSystem>();
    renderSizeSystem->gameViewport = gameViewport;
    renderSizeSystem->Update();

    auto renderSystem = state->ecs.System<RenderSystem>();
    renderSystem->scale = scale;
    renderSystem->renderer = ren;
    renderSystem->Update();
    
    //if (Metadata.ticks() % 30 == 0)
    //    Log("rendered %d entities", nRenderedEntities);

    

    // camera to world pixel offset
    // Vec2 offset = {-gameViewport->world.x * TilePixels + gameViewport->display.x, -gameViewport->world.y * TilePixels + gameViewport->display.y};
    // Draw::player(&state->player, ren, offset);

    int numLinesDrawn = 0;
    if (Debug.settings.drawChunkBorders) {
        numLinesDrawn = Draw::drawChunkBorders(ren, scale, gameViewport);
    } 

    return numRenderedChunks;
}

void highlightTargetedTile(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const GUI* gui, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
        // only draw tile marker if mouse is actually on the world, not on the GUI
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        SDL_FRect playerTileHover = {
            screenPos.x,
            screenPos.y,
            TileWidth,
            TileHeight
        };
        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
        Draw::thickRect(ren, &playerTileHover, round(scale * 2));
    }      
}

void highlightTargetedEntity(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const GUI* gui, GameState* state, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
        // only draw tile marker if mouse is actually on the world, not on the GUI
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        forEachEntityInRange(&state->ecs, &state->chunkmap, playerTargetPos, M_SQRT2, [&](EntityType<PositionComponent> entity){
            if (entity.Has<SizeComponent>()) {
                Vec2 position = *state->ecs.Get<PositionComponent>(entity);
                auto sizeComponent = state->ecs.Get<SizeComponent>(entity);
                Vec2 size = {sizeComponent->width, sizeComponent->height};
                if (
                  playerTargetPos.x > position.x && playerTargetPos.x < position.x + size.x &&
                  playerTargetPos.y > position.y && playerTargetPos.y < position.y + size.y) {
                    // player is targeting this entity
                    Vec2 destPos = gameViewport->worldToPixelPositionF(position);
                    SDL_FRect entityRect = {
                        destPos.x,
                        destPos.y,
                        size.x * TileWidth,
                        size.y * TileHeight
                    };
                    SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                    Draw::thickRect(ren, &entityRect, round(scale * 2));
                    return 1;
                }
            }
            return 0;
        });
    }      
}

void render(SDL_Renderer *ren, float scale, GUI* gui, GameState* state, const GameViewport* gameViewport, Vec2 playerTargetPos) {
    // get window size in pixels
    int renWidth;
    int renHeight;
    SDL_GetRendererOutputSize(ren, &renWidth, &renHeight);

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);

    SDL_RenderSetClipRect(ren, &gameViewport->display);
    int numRenderedChunks = drawWorld(ren, scale, gameViewport, state);
    if (state->player.heldItemStack && ItemData[state->player.heldItemStack->item].flags & Items::Placeable)
        highlightTargetedTile(ren, scale, gameViewport, gui, playerTargetPos);
    highlightTargetedEntity(ren, scale, gameViewport, gui, state, playerTargetPos);
    SDL_RenderSetClipRect(ren, NULL);
    
    gui->draw(ren, scale, gameViewport->display, &state->player);

    //FC_Draw(FreeSans, ren, 5, 5, "rendering %d chunks", numRenderedChunks);
    FC_SetDefaultColor(FreeSans, SDL_RED);
    FC_DrawScale(FreeSans, ren, 5, 5, FC_MakeScale(0.75 * scale, 0.75 * scale), "FPS: %f", Metadata.fps());
    FC_SetDefaultColor(FreeSans, SDL_BLACK);

    //SDL_SetRenderTarget(ren, NULL);
    //SDL_RenderCopy(ren, screenTexture, NULL, NULL);
    //SDL_SetRenderTarget(ren, screenTexture);

    SDL_RenderPresent(ren);
}

#endif