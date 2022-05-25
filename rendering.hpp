#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <vector>
#include <unordered_map>

#include <SDL2/SDL.h>
#include "GameViewport.hpp"
#include "GameState.hpp"
#include "Tiles.hpp"
#include "Metadata.hpp"
#include "Textures.hpp"
#include "Debug.hpp"
#include "GUI.hpp"
#include "NC/colors.h"
#include "Log.hpp"

namespace Draw {

    void thickRect(SDL_Renderer* renderer, const SDL_FRect* rect, int thickness) {
        if (thickness < 2) {
            SDL_RenderDrawRectF(renderer, rect);
        } else {
            SDL_RenderSetScale(renderer, thickness, thickness);
            SDL_FRect sRect = {
                rect->x / thickness,
                rect->y / thickness,
                rect->w / thickness,
                rect->h / thickness
            };
            SDL_RenderDrawRectF(renderer, &sRect);
            SDL_RenderSetScale(renderer, 1, 1);
        }
    }

    void debugRect(SDL_Renderer* renderer, SDL_FRect* rect, float scale) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 200);
        thickRect(renderer, rect, scale);
    }

    void debugHLine(SDL_Renderer* renderer, float y, float x1, float x2, float scale) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderDrawLineF(renderer, x1, y, x2, y);
        if (scale > 1.6f) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 150);
            SDL_RenderDrawLineF(renderer, x1, y + 1, x2, y + 1);
            SDL_RenderDrawLineF(renderer, x1, y - 1, x2, y - 1);
        }
    }

    void debugVLine(SDL_Renderer* renderer, float x, float y1, float y2, float scale) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
        SDL_RenderDrawLineF(renderer, x, y1, x, y2);
        if (scale > 1.6f) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 255, 150);
            SDL_RenderDrawLineF(renderer, x + 1, y1, x + 1, y2);
            SDL_RenderDrawLineF(renderer, x - 1, y1, x +-1, y2);
        }
    }

    void player(const Player* player, SDL_Renderer* renderer, Vec2 offset) {
        float width = 1.0f * TilePixels;
        float height = 1.0f * TilePixels;
        SDL_FRect destination = {
            player->x * TilePixels + offset.x - width/2.0f,
            player->y * TilePixels + offset.y - height/2.0f,
            width,
            height
        };
        SDL_RenderCopyF(renderer, Textures.player, NULL, &destination);
    }

    void chunk(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
        SDL_FRect tileDst;
        float tileWidth = destination.w / CHUNKSIZE;
        float tileHeight = destination.h / CHUNKSIZE;
        tileDst.w = tileWidth;
        tileDst.h = tileHeight;
        tileDst.x = destination.x;
        tileDst.y = destination.y;
        bool warningLogged = false;
        for (int row = 0; row < CHUNKSIZE; row++) {
            for (int col = 0; col < CHUNKSIZE; col++) {
                const Tile* tile = &TILE(chunk, row, col);
                /*
                if (tile->type == TileTypes::Empty) {
                    if (!warningLogged) {
                        Log("Warning: a tile of type Empty needed to be rendered.");
                        warningLogged = true;
                    }
                } else if (tile->type == TileTypes::Space) {} else {*/
                    SDL_Texture* background = TileTypeData[tile->type].background;
                    SDL_RenderCopyF(renderer, background, NULL, &tileDst);
                //}

                tileDst.x += tileWidth;
            }
            // reset tile destination x back to origin at start of new row
            tileDst.x = destination.x;
            tileDst.y += tileHeight;
        }
    }

    void chunkExp(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
        SDL_FRect tileDst;
        tileDst.w = TilePixels;
        tileDst.h = TilePixels;

        for (int type = 0; type < NUM_TILE_TYPES; type++) {
            SDL_Texture* background = TileTypeData[type].background;
            for (int row = 0; row < CHUNKSIZE; row++) {
                for (int col = 0; col < CHUNKSIZE; col++) {
                    int tileType = TILE(chunk, row, col).type;
                    if (tileType == type) {
                        tileDst.x = destination.x + col * tileDst.w;
                        tileDst.y = destination.y + row * tileDst.h;
                        SDL_RenderCopyF(renderer, background, NULL, &tileDst);
                    }
                }
            }
        }
       
    }

    void chunkExp2(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
        SDL_FRect tileDst;
        tileDst.w = TilePixels;
        tileDst.h = TilePixels;


        IVec2 tileTypes[NUM_TILE_TYPES][CHUNKSIZE*CHUNKSIZE];
        int tileTypesSize[NUM_TILE_TYPES] = {0};
        for (int i = 0; i < NUM_TILE_TYPES; i++) {
            //tileTypes[i].reserve(CHUNKSIZE);
        }

        // sort types
        for (int row = 0; row < CHUNKSIZE; row++) {
            for (int col = 0; col < CHUNKSIZE; col++) {
                TileType type = TILE(chunk, row, col).type;
                IVec2 *typeArray = tileTypes[type];
                typeArray[tileTypesSize[type]] = {row, col};
                tileTypesSize[type]++;
            }
        }
        // render each type array one after another
        for (int type = 0; type < NUM_TILE_TYPES; type++) {
            SDL_Texture* background = TileTypeData[type].background;
            for (int i = 0; i < tileTypesSize[type]; i++) {
                tileDst.x = destination.x + tileTypes[type][i].x * tileDst.w;
                tileDst.y = destination.y + tileTypes[type][i].y * tileDst.h;
                SDL_RenderCopyF(renderer, background, NULL, &tileDst);
            }
        }
       
    }

    void simpleChunk(const Chunk* chunk, SDL_Renderer* renderer, SDL_FRect destination) {
        SDL_FRect tileDst;
        float tileWidth = destination.w / CHUNKSIZE;
        float tileHeight = destination.h / CHUNKSIZE;
        tileDst.w = tileWidth;
        tileDst.h = tileHeight;
        for (int row = 0; row < CHUNKSIZE; row++) {
            for (int col = 0; col < CHUNKSIZE; col++) {
                tileDst.x = destination.x + (tileWidth * col);
                tileDst.y = destination.y + (tileHeight * row);
                const Tile* tile = &TILE(chunk, row, col);
                // dont render space or empty tiles
                if (tile->type > 1) {
                    Color backgroundColor = TileTypeData[tile->type].color;
                    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, 255);
                    SDL_RenderFillRectF(renderer, &tileDst);
                }
            }
        }
    }

    int drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport) {
        float distanceToChunkX = fmod(gameViewport->world.x, CHUNKSIZE);
        float firstChunkX = gameViewport->world.x - distanceToChunkX;
        float distanceToChunkY = fmod(gameViewport->world.y, CHUNKSIZE);
        float firstChunkY = gameViewport->world.y - distanceToChunkY;
        Vec2 borderScreenPos = gameViewport->worldToPixelPositionF({firstChunkX, firstChunkY});
        int nLinesDrawn = 0;
        for (; borderScreenPos.x < gameViewport->displayRightEdge(); borderScreenPos.x += CHUNKPIXELS) {
            debugVLine(renderer, borderScreenPos.x, gameViewport->display.y, gameViewport->displayBottomEdge(), scale);
            nLinesDrawn++;
        }
        for (; borderScreenPos.y < gameViewport->displayBottomEdge(); borderScreenPos.y += CHUNKPIXELS) {
            // SDL_RenderDrawLineF(renderer, gameViewport->display.x, borderScreenPos.y, gameViewport->displayRightEdge(), borderScreenPos.y);
            debugHLine(renderer, borderScreenPos.y, gameViewport->display.x, gameViewport->displayRightEdge(), scale);
            nLinesDrawn++;
        }
        return nLinesDrawn;
    }

    int world(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, GameState* state) {
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
                    CHUNKPIXELS,
                    CHUNKPIXELS
                };
                
                // When zoomed out far, simplify rendering for better performance and less distraction
                if (TilePixels < 10.0f) {
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

        // calculate destination rects for render components
        // All position values set
        state->ecs.iterateEntities(
        [](Uint32 signature){
            Uint32 need = componentSignature<RenderComponent, PositionComponent, SizeComponent>();
            if (((signature & need) == need) && !(signature & componentSignature<CenteredRenderFlagComponent>())) {
                return true;
            }
            return false;
        },
        [gameViewport](ECS* ecs, Entity entity){
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            Vec2 position = gameViewport->worldToPixelPositionF(*ecs->Get<PositionComponent>(entity));
            renderDst->x = position.x;
            renderDst->y = position.y;
        });

        // all size values set
        state->ecs.iterateEntities<RenderComponent, PositionComponent, SizeComponent>(
        [](ECS* ecs, Entity entity){
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            renderDst->w = ecs->Get<SizeComponent>(entity)->width * TilePixels;
            renderDst->h = ecs->Get<SizeComponent>(entity)->height * TilePixels;
        });

        state->ecs.iterateEntities<CenteredRenderFlagComponent, RenderComponent, PositionComponent, SizeComponent>(
        [gameViewport](ECS* ecs, Entity entity){
            SDL_FRect* renderDst = &ecs->Get<RenderComponent>(entity)->destination;
            Vec2 position = gameViewport->worldToPixelPositionF(*ecs->Get<PositionComponent>(entity));
            renderDst->x = position.x - renderDst->w / 2.0f;
            renderDst->y = position.y - renderDst->w / 2.0f;
        });

        // draw simple render entities

        struct RenderTarget {
            SDL_Texture* texture;
            SDL_FRect* destination;
        };

        //std::vector<RenderTarget> visibleRenders;
        std::unordered_map<int, std::unordered_map<SDL_Texture*, std::vector<RenderTarget>>> rendersMaps;

        // cull non visible entities
        // and sort textures
        /*
        state->ecs.iterateEntities<RenderComponent>(
        [&rendersMaps, gameViewport](ECS* ecs, Entity entity){
            RenderComponent* render = ecs->Get<RenderComponent>(entity);
            if (render->texture != NULL) {
                SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    //(dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge() &&
                    //dest->y > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    rendersMaps[render->layer][render->texture].push_back({render->texture, dest});
                }
            }
        });

        int nRenderedEntities = 0;
        for (int i = -10; i < 10; i++) {
            std::unordered_map<SDL_Texture*, std::vector<RenderTarget>> rendersMap = rendersMaps[i];
            for (auto& renderList : rendersMap) {
                for (auto& render : renderList.second) {
                    SDL_RenderCopyF(ren, render.texture, NULL, render.destination);
                    nRenderedEntities++;
                }
            }
        }
        
        /*/

        std::vector<RenderTarget> renderLayers[NUM_RENDER_LAYERS];

        state->ecs.iterateEntities<RenderComponent>(
        [&renderLayers, gameViewport](ECS* ecs, Entity entity){
            RenderComponent* render = ecs->Get<RenderComponent>(entity);
            if (render->texture != NULL) {
                SDL_FRect* dest = &render->destination;
                if ((dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge()) &&
                    (dest->y + dest->h > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    //(dest->x + dest->w > gameViewport->display.x && dest->x < gameViewport->displayRightEdge() &&
                    //dest->y > gameViewport->display.y && dest->y < gameViewport->displayBottomEdge())) {
                    
                    renderLayers[render->layer].push_back({render->texture, dest});
                }
            }
        });
        //*/
        int nRenderedEntities = 0;
        for (int l = 0; l < NUM_RENDER_LAYERS; l++) {
            std::vector<RenderTarget>& renderLayer = renderLayers[l];
            for (size_t r = 0; r < renderLayer.size(); r++) {
                SDL_RenderCopyF(ren, renderLayer[r].texture, NULL, renderLayer[r].destination);
                nRenderedEntities++;
            }
        }

        /*
        int nRenderedEntities = 0;
        state->ecs.iterateComponents<RenderComponent>(
        [ren, &nRenderedEntities](RenderComponent* render){
            SDL_RenderCopyF(ren, render->texture, NULL, &render->destination);
            nRenderedEntities++;
        });
        */
        
        if (Metadata.ticks() % 30 == 0)
            Log("rendered %d entities", nRenderedEntities);
        

        if (Debug.settings.drawEntityIDs) {
            state->ecs.iterateEntities<PositionComponent>(
            [ren, gameViewport](ECS* ecs, Entity entity){
                // const GrowthComponent* growth = ecs->Get<GrowthComponent>(entity);
                const PositionComponent* position = ecs->Get<PositionComponent>(entity);
                Vec2 screenPos = gameViewport->worldToPixelPositionF({position->x, position->y});
                FC_DrawScale(FreeSans, ren, screenPos.x, screenPos.y, FC_MakeScale(0.5, 0.5), "%u", entity);
            });
        }

        if (Debug.settings.drawEntityRects) {
            // draw simple render entities
            state->ecs.iterateComponents<RenderComponent>(
            [ren, scale](RenderComponent* render){
                Draw::debugRect(ren, &render->destination, scale);
            });
        }

        // camera to world pixel offset
        Vec2 offset = {-gameViewport->world.x * TilePixels + gameViewport->display.x, -gameViewport->world.y * TilePixels + gameViewport->display.y};
        Draw::player(&state->player, ren, offset);

        int numLinesDrawn = 0;
        if (Debug.settings.drawChunkBorders) {
            numLinesDrawn = drawChunkBorders(ren, scale, gameViewport);
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
                TilePixels,
                TilePixels
            };
            SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
            Draw::thickRect(ren, &playerTileHover, round(scale * 2));
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
    int numRenderedChunks = Draw::world(ren, scale, gameViewport, state);
    Draw::highlightTargetedTile(ren, scale, gameViewport, gui, playerTargetPos);
    SDL_RenderSetClipRect(ren, NULL);
    
    gui->draw(ren, scale, gameViewport->display, &state->player);

    //FC_Draw(FreeSans, ren, 5, 5, "rendering %d chunks", numRenderedChunks);
    FC_SetDefaultColor(FreeSans, SDL_RED);
    FC_DrawScale(FreeSans, ren, 5, 5, FC_MakeScale(0.75 * scale, 0.75 * scale), "FPS: %f", Metadata.fps());
    FC_SetDefaultColor(FreeSans, SDL_BLACK);

    SDL_RenderPresent(ren);
}

#endif