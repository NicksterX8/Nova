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
#include "Entities/Systems/Rendering.hpp"
#include "Entities/Systems/Systems.hpp"
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

    ds.drawChunkBorders = true;
    ds.drawChunkCoordinates = false;
    ds.drawEntityRects = false;
    ds.drawEntityIDs = false;

    Debug.settings = ds;
}

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

int main() {
    setDebugSettings();

    void* args = NULL;
    SDL_LogSetOutputFunction(CustomLog, args);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
    SDL_LogSetPriority(LOG_CATEGORY_MAIN, SDL_LOG_PRIORITY_INFO);

    LogCategory = LOG_CATEGORY_MAIN;

    SDLContext sdlCtx = initSDL();
    //screenTexture = SDL_CreateTexture(sdlCtx.ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 1920, 1080);
    //SDL_SetRenderTarget(sdlCtx.ren, screenTexture);

    int screenWidth,screenHeight;
    SDL_GetRendererOutputSize(sdlCtx.ren, &screenWidth, &screenHeight);

    load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

    LogCategory = LOG_CATEGORY_MAIN;

    Log("Number of systems: %d", NUM_SYSTEMS);

    {
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

    {
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

    GameViewport gameViewport;

    GameState* state = new GameState();
    
    state->init(sdlCtx.ren, &gameViewport);

    

    /*
    auto renderPositionSystem = new RenderPositionSystem();
    state->ecs.NewSystem<RenderPositionSystem>(renderPositionSystem);
    auto renderSizeSystem = new RenderSizeSystem();
    state->ecs.NewSystem<RenderSizeSystem>(renderSizeSystem);

    auto rotationSystem = new RotationSystem();
    state->ecs.NewSystem<RotationSystem>(rotationSystem);

    auto motionSystem = new MotionSystem();
    state->ecs.NewSystem<MotionSystem>(motionSystem);
    state->ecs.NewSystem<ExplosivesSystem>(new ExplosivesSystem());
    state->ecs.NewSystem<GrowthSystem>(new GrowthSystem());
    state->ecs.NewSystem<DyingSystem>(new DyingSystem());
    */

    

    for (int e = 0; e < 500; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        Entity entity = Entities::Tree(&state->ecs, pos, {randomInt(1, 5) + rand0to1(), randomInt(1, 5) + rand0to1()});
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