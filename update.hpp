#ifndef UPDATE_INCLUDED
#define UPDATE_INCLUDED

#include <vector>
#include <random>
#include <SDL2/SDL.h>
#include <math.h>

#include "SDL_FontCache/SDL_FontCache.h"
#include "NC/SDLContext.h"

#include "constants.hpp"
#include "Textures.hpp"
#include "GameState.hpp"
#include "GameViewport.hpp"
#include "PlayerControls.hpp"
#include "GUI.hpp"
#include "sdl.hpp"

struct Context {
    SDLContext& sdlCtx;
    const Uint8 *keyboard;
    GameState* state;
    GUI* gui;
    PlayerControls* playerControls;
    GameViewport* gameViewport;
    float& worldScale;
    MouseState& lastUpdateMouseState;
    Vec2& lastUpdatePlayerTargetPos;
    DebugClass& debug;
    MetadataTracker& metadata;

    Context(SDLContext& sdlContext, GameState* state, GUI* gui, PlayerControls& playerControls, GameViewport* gameViewport,
    float& worldScale, MouseState& lastUpdateMouseState, Vec2& lastUpdatePlayerTargetPos,
    DebugClass& debug, MetadataTracker& metadata):
    sdlCtx(sdlContext), state(state), gui(gui), gameViewport(gameViewport), worldScale(worldScale),
    lastUpdateMouseState(lastUpdateMouseState), lastUpdatePlayerTargetPos(lastUpdatePlayerTargetPos),
    debug(debug), metadata(metadata) {
        keyboard = SDL_GetKeyboardState(NULL);
        this->playerControls = &playerControls;
    }
};

Vec2 getMouseWorldPosition(const GameViewport& gameViewport);

GameViewport newGameViewport(int renderWidth, int renderHeight, float focusX, float focusY);

// Main game loop
int update(Context ctx);

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param);

#endif