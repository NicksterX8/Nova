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