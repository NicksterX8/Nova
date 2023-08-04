#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_rect.h>

#define GAME_TITLE "Faketorio"
#define WINDOW_TITLE "Faketorio"

/* Things */
#define GetTicks SDL_GetTicks
#define GetPerformanceCounter SDL_GetPerformanceCounter
#define GetPerformanceFrequency SDL_GetPerformanceFrequency

#define PLAYER_INVENTORY_SIZE 32

#define NUM_RENDER_LAYERS 16
#define CHUNKSIZE 64
static_assert(CHUNKSIZE > 0, "Chunks can't be empty");

#define BASE_UNIT_SCALE 32.0f

const float PLAYER_SPEED = 0.3f;
const float PLAYER_ROTATION_SPEED = 1.0f;

#define TARGET_FPS 60
#define ENABLE_VSYNC 1

const float PLAYER_DIAMETER = 0.8f;

namespace RenderLayers {
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

using RenderLayer = RenderLayers::Layers;

#define DEBUG 1
#define BOUNDS_CHECKS 1

#endif