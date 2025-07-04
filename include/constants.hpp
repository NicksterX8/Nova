#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_rect.h>

#define GAME_TITLE "Faketorio"
#define WINDOW_TITLE "Faketorio"

#define USE_SECONDARY_WINDOW 0
#define WINDOW_HIGH_DPI 0

#define USE_MULTITHREADING 1
#define CACHE_LINE_SIZE 128

#define GetTicks SDL_GetTicks
#define GetPerformanceCounter SDL_GetPerformanceCounter
#define GetPerformanceFrequency SDL_GetPerformanceFrequency

#define PLAYER_INVENTORY_SIZE 32

#define NUM_RENDER_LAYERS 16
#define CHUNKSIZE 64
static_assert(CHUNKSIZE > 0, "Chunks can't be empty");

#define BASE_UNIT_SCALE 32.0f

const float PLAYER_SPEED = 0.11f;
const float PLAYER_ROTATION_SPEED = 1.0f;

#define TARGET_FPS 60
#define TICKS_PER_SECOND 60
#define ENABLE_VSYNC 0

const float PLAYER_DIAMETER = 0.8f;

namespace RenderLayers {
    enum Layers {
        Lowest,
        Water,
        Tilemap,
        Shadows,
        Items,
        Buildings,
        Particles,
        Player,
        Trees,
        Highest
    };
}

using RenderLayer = RenderLayers::Layers;

#define DEBUG 1
#define BOUNDS_CHECKS 1

#endif