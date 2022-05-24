#include "constants.hpp"
#include "Log.hpp"

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "SDL2_gfx/SDL2_gfxPrimitives.h"
#include "SDL_FontCache/SDL_FontCache.h"

#include "NC/SDLContext.h"
#include "NC/SDLBuild.h"
#include "NC/colors.h"
#include "NC/utils.h"

#include "rendering.hpp"
#include "Textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GameViewport.hpp"
#include "GUI.hpp"
#include "Debug.hpp"
#include "loadData.hpp"
#include "Entity.hpp"
#include "update.hpp"

#define DEBUG
#ifdef DEBUG
    #include "Testing.hpp"
#endif

#define WINDOW_HIGH_DPI 1

// load assets and stuff
int load(SDL_Renderer *ren, float renderScale = 1.0f) {
    FreeSans = FC_CreateFont();
    FC_LoadFont(FreeSans, ren, "assets/fonts/FreeSans.ttf", 36, SDL_BLACK, TTF_STYLE_NORMAL);

    Textures.load(ren);
    
    return 0;
}

// unload assets and stuff
void unload() {
    FC_FreeFont(FreeSans);

    Textures.unload();
}

void setDebugSettings() {
    // shorten name for easier typing while keeping default settings
    DebugSettings ds = Debug.settings;

    ds.drawChunkBorders = false;
    ds.drawChunkCoordinates = false;
    ds.drawEntityRects = true;
    ds.drawEntityIDs = false;

    Debug.settings = ds;
}

SDLContext initSDL() {
    SDLSettings settings = DefaultSDLSettings;
    settings.vsync = ENABLE_VSYNC;
    settings.windowTitle = "Faketorio";
    settings.windowIconPath = "assets/bad-factorio-logo.png";
    if (WINDOW_HIGH_DPI) {
        settings.allowHighDPI = true;
    }
    SDLContext sdlCtx = initSDLAndContext(&settings); // SDL Context

    return sdlCtx;
}

int main() {
    setDebugSettings();

    void* args = NULL;
    SDL_LogSetOutputFunction(CustomLog, args);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
    SDL_LogSetPriority(LOG_CATEGORY_MAIN, SDL_LOG_PRIORITY_INFO);

    LogCategory = LOG_CATEGORY_DEBUG;

    SDLContext sdlCtx = initSDL();
    int screenWidth,screenHeight;
    SDL_GetRendererOutputSize(sdlCtx.ren, &screenWidth, &screenHeight);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

#ifdef DEBUG
    Tests::runAll();
    Log("size of tile: %lu\n", sizeof(Tile));
    Log("size of ECS: %lu\n", sizeof(ECS));
#endif

    LogCategory = LOG_CATEGORY_MAIN;

    GameState* state = new GameState();
    state->init();

    GameViewport gameViewport = newGameViewport(screenWidth, screenHeight, state->player.x, state->player.y);
    float worldScale = 1.0f;
    SDL_Point mousePos = SDLGetMousePixelPosition();
    MouseState lastUpdateMouseState = {
        mousePos.x,
        mousePos.y,
        SDLGetMouseButtons()
    };
    Vec2 lastUpdatePlayerTargetPos = gameViewport.pixelToWorldPosition(mousePos.x, mousePos.y);
    GUI gui;
    Context context = Context(
        sdlCtx,
        state,
        &gui,
        &gameViewport,
        worldScale,
        lastUpdateMouseState,
        lastUpdatePlayerTargetPos
    );

    if (Metadata.vsyncEnabled) {
        Metadata.setTargetFps(60);
    } else {
        Metadata.setTargetFps(TARGET_FPS);
    }
    Metadata.start();
    NC_SetMainLoop(updateWrapper, &context);

    unload();

    state->free();

    quitSDLContext(sdlCtx);

    return 0;
}