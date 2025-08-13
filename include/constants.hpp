#ifndef CONSTANTS_INCLUDED
#define CONSTANTS_INCLUDED

#include <stdio.h>
#include <stdarg.h>
#include <SDL3/SDL_log.h>

#define GAME_TITLE "Nova"
#define WINDOW_TITLE "Nova"

#define PATH_TO_SOURCE "/Users/nick/Faketorio/src"
#define PATH_TO_INCLUDE "/Users/nick/Faketorio/../faketorio/include"

#define USE_SECONDARY_WINDOW 0
#define WINDOW_HIGH_DPI 1
#define TARGET_FPS 240
#define TICKS_PER_SECOND 60
#define ENABLE_VSYNC 1

#define USE_MULTITHREADING 1

#define GetTicks SDL_GetTicks
#define GetPerformanceCounter SDL_GetPerformanceCounter
#define GetPerformanceFrequency SDL_GetPerformanceFrequency

#define PLAYER_INVENTORY_SIZE 32

#define NUM_RENDER_LAYERS 16
#define CHUNKSIZE 128
static_assert(CHUNKSIZE > 0, "Chunks can't be empty");

#define BASE_UNIT_SCALE 32.0f

const float PLAYER_SPEED = 0.11f;
const float PLAYER_ROTATION_SPEED = 1.0f;

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
        Text,
        Highest
    };
}

using RenderLayer = RenderLayers::Layers;

#endif