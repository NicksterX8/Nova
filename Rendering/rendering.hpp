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
#include "../SDL2_gfx/SDL2_gfxPrimitives.h"
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
            const ChunkData* chunkdata = state->chunkmap.getChunkData({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.createChunk({x, y});
                generateChunk(newChunk->chunk);
                chunkdata = newChunk;
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
                Draw::simpleChunk(chunkdata->chunk, ren, destination);
            } else {
                Draw::chunkExp(chunkdata->chunk, ren, scale, destination);
                // draw chunk coordinates
                if (Debug->settings.drawChunkCoordinates) {
                    FC_DrawScale(FreeSans, ren, destination.x + 3*scale, destination.y + 2*scale, FC_MakeScale(0.5f,0.5f),
                    "%d, %d", x * CHUNKSIZE, y * CHUNKSIZE);
                }

                if (Debug->settings.drawChunkEntityCount) {
                    FC_DrawScale(FreeSans, ren, destination.x + destination.w/2.0f, destination.y + destination.h/2.0f, FC_MakeScale(0.5f,0.5f),
                    "%llu", chunkdata->closeEntities.size());
                }
            }

        }
    }

    auto renderSystem = SimpleRectRenderSystem(ren, gameViewport);
    renderSystem.scale = scale;
    renderSystem.Update(state->ecs, state->chunkmap);

    using RenderSystemQuery = ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Position, EC::Size>,
        ECS::AvoidComponents<>,
        ECS::LogicalOrComponents<>
    >;

    state->ecs.ForEach<RenderSystemQuery>([&](Entity entity){
        //Log("entity");
    });
    
    //if (Metadata.ticks() % 30 == 0)
    //    Log("rendered %d entities", nRenderedEntities);

    if (Debug->settings.drawChunkBorders) {
        Draw::drawChunkBorders(ren, scale, gameViewport);
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

void highlightTargetedEntity(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const GUI* gui, const GameState* state, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
    // only draw tile marker if mouse is actually on the world, not on the GUI
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        OptionalEntity<> targetedEntity = state->player.selectedEntity;
        if (targetedEntity.Exists(&state->ecs)) {
            if (targetedEntity.Has<EC::Render>(&state->ecs)) {
                SDL_FRect entityRect = targetedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, &entityRect, round(scale * 2));
            }
        } else {
            /*
            forEachEntityInRange(&state->ecs, &state->chunkmap, playerTargetPos, M_SQRT2, [&](EntityT<EC::Position> entity){
                if (entity.Has<EC::Size, EC::Render>()) {
                    Vec2 position = *state->ecs.Get<EC::Position>(entity);
                    auto sizeComponent = state->ecs.Get<EC::Size>(entity);
                    Vec2 size = {sizeComponent->width, sizeComponent->height};
                    if (
                        playerTargetPos.x > position.x && playerTargetPos.x < position.x + size.x &&
                        playerTargetPos.y > position.y && playerTargetPos.y < position.y + size.y) {
                        // player is targeting this entity
                        Vec2 destPos = gameViewport->worldToPixelPositionF(position);
                        SDL_FRect entityRect = entity.Get<EC::Render>()->destination;
                        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                        Draw::thickRect(ren, &entityRect, round(scale * 2));
                        return 1;
                    }
                }
                return 0;
            });
            */
            auto focusedEntity = findPlayerFocusedEntity(&state->ecs, state->chunkmap, playerTargetPos);
            if (focusedEntity != NullEntity) {
                SDL_FRect* entityRect = &focusedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, entityRect, round(scale * 2));
            }
        }
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

    FC_SetDefaultColor(FreeSans, SDL_RED);
    FC_DrawScale(FreeSans, ren, 5, 5, FC_MakeScale(0.75 * scale, 0.75 * scale), "FPS: %f", Metadata->fps());
    FC_SetDefaultColor(FreeSans, SDL_BLACK);

    SDL_RenderPresent(ren);
}

#endif