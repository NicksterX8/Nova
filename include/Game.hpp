#ifndef UPDATE_INCLUDED
#define UPDATE_INCLUDED

#include <vector>
#include <random>
#include <SDL2/SDL.h>
#include <math.h>

#include "constants.hpp"
#include "rendering/textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GUI/Gui.hpp"
#include "sdl.hpp"
#include "Camera.hpp"
#include "rendering/rendering.hpp"

struct Game;

void updateTilePixels(float scale);

void placeInserter(ChunkMap& chunkmap, EntityWorld* ecs, Vec2 mouseWorldPos);

void rotateEntity(const ComponentManager<EC::Rotation, EC::Rotatable>& ecs, EntityT<EC::Rotation, EC::Rotatable> entity, bool counterClockwise);

void setDefaultKeyBindings(Game& ctx, PlayerControls* controls);

struct Game {
    SDLContext sdlCtx;
    const Uint8 *keyboard;
    GameState* state;
    Camera camera;
    Gui* gui;
    PlayerControls* playerControls;
    float worldScale;
    MouseState lastUpdateMouseState;
    Vec2 lastUpdatePlayerTargetPos;
    DebugClass* debug;
    MetadataTracker metadata;
    RenderContext* renderContext;
    Mode mode;

    Game(SDLContext sdlContext):
    sdlCtx(sdlContext), metadata(TARGET_FPS, true) {
        debug = new DebugClass();
        Debug = debug;

        keyboard = SDL_GetKeyboardState(NULL);
        state = NULL;
        gui = NULL;
        playerControls = NULL;
        renderContext = NULL;
        mode = Unstarted;
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
            LogError("Parameter passed to emscripten update wrapper was null!");
            return -1;
        }
        
    }
#endif

    void setMode(Mode mode) {
        this->mode = mode;
    }

};

// update wrapper function to unwrap the void pointer main loop parameter into its properties
int updateWrapper(void *param);

#endif