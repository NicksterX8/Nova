#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_rect.h>

#define GAME_TITLE "Faketorio"
#define WINDOW_TITLE "Faketiorio"

/* Things */
#define GetTicks SDL_GetTicks
#define GetPerformanceCounter SDL_GetPerformanceCounter
#define GetPerformanceFrequency SDL_GetPerformanceFrequency
#define Rect SDL_Rect
#define FRect SDL_FRect
#define Color SDL_Color

#define PLAYER_INVENTORY_SIZE 32

#define NUM_RENDER_LAYERS 16
#define CHUNKSIZE 32

#define DEFAULT_TILE_PIXEL_SIZE 32.0f
#define TILE_PIXEL_VERTICAL_SCALE 1

const float PLAYER_SPEED = 0.3;

#define TARGET_FPS 60
#define ENABLE_VSYNC 1

const float PLAYER_DIAMETER = 0.8f;

namespace RenderLayer {
    enum Layers {
        Lowest,
        Water,
        Items,
        Buildings,
        Particles,
        Trees,
        Player,
        Highest
    };
}

#define DEBUG 1

#endif