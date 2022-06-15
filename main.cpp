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
#include "NC/cpp-vectors.hpp"

#include "Textures.hpp"
#include "GameState.hpp"
#include "PlayerControls.hpp"
#include "GameViewport.hpp"
#include "GUI.hpp"
#include "Debug.hpp"
#include "loadData.hpp"
#include "Entities/Entities.hpp"
#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"
#include "update.hpp"
#include "Debug.hpp"
#include "Items.hpp"
#include "Log.hpp"
#include "constants.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

#define WINDOW_HIGH_DPI 0

SDLContext initSDL() {
    SDLSettings settings = DefaultSDLSettings;
    settings.vsync = ENABLE_VSYNC;
    settings.windowTitle = "Faketorio";
    settings.windowIconPath = "assets/bad-factorio-logo.png";
    settings.windowWidth = 1000;
    settings.windowHeight = 800;
    if (WINDOW_HIGH_DPI) {
        settings.allowHighDPI = true;
    }
    SDLContext sdlCtx = initSDLAndContext(&settings); // SDL Context

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    return sdlCtx;
}

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
    ds.drawEntityRects = false;
    ds.drawEntityIDs = false;

    Debug.settings = ds;
}

void setDefaultKeyBindings(PlayerControls* controls) {
    controls->addKeyBinding(new FunctionKeyBinding('y', [](Context* ctx){
        auto mouse = ctx->playerControls->getMouse();
        Entity zombie = Entities::Enemy(
            &ctx->state->ecs,
            ctx->gameViewport->pixelToWorldPosition(mouse.x, mouse.y),
            ctx->state->player.entity
        );
    }));

    KeyBinding* keyBindings[] = {
        new ToggleKeyBinding('b', &Debug.settings.drawChunkBorders),
        new ToggleKeyBinding('u', &Debug.settings.drawEntityRects),
        new ToggleKeyBinding('c', &Debug.settings.drawChunkCoordinates),

        new FunctionKeyBinding('q', [](Context* ctx){
            ctx->state->player.releaseHeldItem();
        }),
        new FunctionKeyBinding('l', [](Context* ctx){
            // do airstrikes row by row
            for (int y = -100; y < 100; y += 5) {
                for (int x = -100; x < 100; x += 5) {
                    auto airstrike = Entities::Airstrike(&ctx->state->ecs, Vec2(x, y * 2), {3.0f, 3.0f}, Vec2(x, y));
                }
            }
        })

    };

    for (size_t i = 0; i < sizeof(keyBindings) / sizeof(KeyBinding*); i++) {
        controls->addKeyBinding(keyBindings[i]);
    }
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

void logEntitySystemInfo() {
    Log("Number of systems: %d", NUM_SYSTEMS);
    const char* systemsStr = TOSTRING((SYSTEMS));
    int length = strlen(systemsStr);
    char systemsStr2[length];
    strcpy(systemsStr2, systemsStr+1);
    systemsStr2[length-2] = '\0';

    char (systemsList[NUM_SYSTEMS])[256];
    int charIndex = 0;
    for (int i = 0; i < NUM_SYSTEMS; i++) {
        const char* start = &systemsStr2[charIndex];
        const char* end;
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                end = chr;
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                end = chr;
                break;
            }
            chr++;
            size++;
        }
        strlcpy(systemsList[i], start, size+1);
        systemsList[i][size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_SYSTEMS; i++) {
        Log("ID: %d, System: %s.", i, systemsList[i]);
    }
}

void logEntityComponentInfo() {
    const char* componentsStr = TOSTRING((COMPONENTS));
    int length = strlen(componentsStr);
    char componentsStr2[length];
    strcpy(componentsStr2, componentsStr+1);
    componentsStr2[length-2] = '\0';

    char (componentsList[NUM_COMPONENTS])[256];
    int charIndex = 0;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const char* start = &componentsStr2[charIndex];
        const char* end;
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                end = chr;
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                end = chr;
                break;
            }
            chr++;
            size++;
        }
        strlcpy(componentsList[i], start, size+1);
        componentsList[i][size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_COMPONENTS; i++) {
        Log("ID: %d, Component: %s.", i, componentsList[i]);
        componentNames[i] = componentsList[i];
    }
}

int main(int argc, char** argv) {
    setDebugSettings();

    bool useConsoleColorCodes = true;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes")) {
            useConsoleColorCodes = false;
        }
    }

    LogArguments* logArgs = new LogArguments(useConsoleColorCodes);
    SDL_LogSetOutputFunction(CustomLog, logArgs);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
    SDL_LogSetPriority(LOG_CATEGORY_MAIN, SDL_LOG_PRIORITY_INFO);

    LogCategory = LOG_CATEGORY_MAIN;

    SDLContext sdlCtx = initSDL();

    logEntitySystemInfo();
    logEntityComponentInfo();

    int screenWidth,screenHeight;
    SDL_GetRendererOutputSize(sdlCtx.ren, &screenWidth, &screenHeight);

    load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

    GameViewport gameViewport;

    GameState* state = new GameState();
    
    state->init(sdlCtx.ren, &gameViewport);

    for (int e = 0; e < 1000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        Entity entity = Entities::Tree(&state->ecs, pos, {1, 1});
    }

    gameViewport = newGameViewport(screenWidth, screenHeight, state->player.getPosition().x, state->player.getPosition().y);
    float worldScale = 1.0f;
    SDL_Point mousePos = SDLGetMousePixelPosition();
    MouseState lastUpdateMouseState = {
        mousePos.x,
        mousePos.y,
        SDLGetMouseButtons()
    };
    Vec2 lastUpdatePlayerTargetPos = gameViewport.pixelToWorldPosition(mousePos.x, mousePos.y);
    GUI gui;
    PlayerControls playerControls = PlayerControls(gameViewport); // player related event handler
    setDefaultKeyBindings(&playerControls);
    Context context = Context(
        sdlCtx,
        state,
        &gui,
        playerControls,
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
    double secondsElapsed = Metadata.end();
    Log("Time elapsed: %.1f", secondsElapsed);

    unload();

    state->free();

    quitSDLContext(sdlCtx);

    return 0;
}