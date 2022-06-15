#include "Drawing.hpp"
#include "../GameViewport.hpp"

void Draw::thickRect(SDL_Renderer* renderer, const SDL_FRect* rect, int thickness) {
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

void Draw::debugRect(SDL_Renderer* renderer, const SDL_FRect* rect, float scale) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 200);
    thickRect(renderer, rect, scale);
}

void Draw::debugHLine(SDL_Renderer* renderer, float y, float x1, float x2, float scale) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderDrawLineF(renderer, x1, y, x2, y);
    if (scale > 1.6f) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 150);
        SDL_RenderDrawLineF(renderer, x1, y + 1, x2, y + 1);
        SDL_RenderDrawLineF(renderer, x1, y - 1, x2, y - 1);
    }
}

void Draw::debugVLine(SDL_Renderer* renderer, float x, float y1, float y2, float scale) {
    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
    SDL_RenderDrawLineF(renderer, x, y1, x, y2);
    if (scale > 1.6f) {
        SDL_SetRenderDrawColor(renderer, 255, 0, 255, 150);
        SDL_RenderDrawLineF(renderer, x + 1, y1, x + 1, y2);
        SDL_RenderDrawLineF(renderer, x - 1, y1, x +-1, y2);
    }
}

void Draw::player(const Player* player, SDL_Renderer* renderer, Vec2 offset) {
    float width = 1.0f * TileWidth;
    float height = 1.0f * TileHeight;
    Vec2 playerPosition = player->getPosition();
    SDL_FRect destination = {
        playerPosition.x * TileWidth + offset.x - width/2.0f,
        playerPosition.y * TileHeight + offset.y - height/2.0f,
        width,
        height
    };
    SDL_RenderCopyF(renderer, Textures.player, NULL, &destination);
}

void Draw::chunk(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
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

void Draw::chunkExp(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
    SDL_FRect tileDst;
    tileDst.w = TileWidth;
    tileDst.h = TileHeight;

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

void Draw::chunkExp2(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination) {
    SDL_FRect tileDst;
    tileDst.w = TileWidth;
    tileDst.h = TileHeight;


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

void Draw::simpleChunk(const Chunk* chunk, SDL_Renderer* renderer, SDL_FRect destination) {
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

int Draw::drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport) {
    float distanceToChunkX = fmod(gameViewport->world.x, CHUNKSIZE);
    float firstChunkX = gameViewport->world.x - distanceToChunkX;
    float distanceToChunkY = fmod(gameViewport->world.y, CHUNKSIZE);
    float firstChunkY = gameViewport->world.y - distanceToChunkY;
    Vec2 borderScreenPos = gameViewport->worldToPixelPositionF({firstChunkX, firstChunkY});
    int nLinesDrawn = 0;
    for (; borderScreenPos.x < gameViewport->displayRightEdge(); borderScreenPos.x += TileWidth * CHUNKSIZE) {
        debugVLine(renderer, borderScreenPos.x, gameViewport->display.y, gameViewport->displayBottomEdge(), scale);
        nLinesDrawn++;
    }
    for (; borderScreenPos.y < gameViewport->displayBottomEdge(); borderScreenPos.y += TileHeight * CHUNKSIZE) {
        // SDL_RenderDrawLineF(renderer, gameViewport->display.x, borderScreenPos.y, gameViewport->displayRightEdge(), borderScreenPos.y);
        debugHLine(renderer, borderScreenPos.y, gameViewport->display.x, gameViewport->displayRightEdge(), scale);
        nLinesDrawn++;
    }
    return nLinesDrawn;
}