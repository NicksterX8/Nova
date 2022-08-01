#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <SDL2/SDL.h>
#include "../GameState.hpp"
#include "../GameViewport.hpp"

namespace Draw {
    void thickRect(SDL_Renderer* renderer, const SDL_FRect* rect, int thickness);

    void debugRect(SDL_Renderer* renderer, const SDL_FRect* rect, float scale);

    void debugHLine(SDL_Renderer* renderer, float y, float x1, float x2, float scale);

    void debugVLine(SDL_Renderer* renderer, float x, float y1, float y2, float scale);

    void player(const Player* player, SDL_Renderer* renderer, Vec2 offset);

    void chunk(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination);

    void chunkExp(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination);

    void chunkExp2(const Chunk* chunk, SDL_Renderer* renderer, float scale, SDL_FRect destination);

    void simpleChunk(const Chunk* chunk, SDL_Renderer* renderer, SDL_FRect destination);

    // @return The number of lines drawn on the screen
    int drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport);
}

#endif