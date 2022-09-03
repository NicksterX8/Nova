#ifndef UPDATE_INCLUDED
#define UPDATE_INCLUDED

#include <vector>
#include <random>
#include <SDL2/SDL.h>
#include <math.h>

#include "NC/SDLContext.h"

#include "constants.hpp"
#include "Textures.hpp"
#include "GameState.hpp"
#include "GameViewport.hpp"
#include "PlayerControls.hpp"
#include "GUI.hpp"
#include "sdl.hpp"
#include "Camera.hpp"

struct Game;
struct RenderContext;

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos);

void rotateEntity(const ComponentManager<EC::Rotation, EC::Rotatable>& ecs, EntityT<EC::Rotation, EC::Rotatable> entity, bool counterClockwise);

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls);

struct Game {
    SDLContext& sdlCtx;
    const Uint8 *keyboard;
    GameState* state;
    Camera camera;
    GUI* gui;
    PlayerControls* playerControls;
    float worldScale;
    MouseState lastUpdateMouseState;
    Vec2 lastUpdatePlayerTargetPos;
    DebugClass* debug;
    MetadataTracker metadata;
    RenderContext* renderContext;

    Game(SDLContext& sdlContext):
    sdlCtx(sdlContext), metadata(TARGET_FPS, true) {
        debug = new DebugClass();

        keyboard = SDL_GetKeyboardState(NULL);
        state = NULL;
        gui = NULL;
        playerControls = NULL;
        renderContext = NULL;
    }

    void init(int screenWidth, int screenHeight);

    void start();

    void quit();

    void destroy();

    int update();
#ifdef EMSCRIPTEN
    static int emscriptenUpdateWrapper(void* param) {
        Game* game = static_cast<Game*>(param);
        if (game) {
            return game->update();
        } else {
            // handle error
            return -1;
        }
        
    }
#endif
};

Vec2 getMouseWorldPosition(const Camera& camera);

GameViewport newGameViewport(int renderWidth, int renderHeight, float focusX, float focusY);

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param);

#endif