#include "constants.hpp"
#include "Log.hpp"

#include <vector>
#include <stdio.h>
#include <libproc.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "gl.h"

#include "SDL2_gfx/SDL2_gfxPrimitives.h"

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

#include "EntityComponents/Components.hpp"
#include "Entities/Entities.hpp"
#include "EntitySystems/Rendering.hpp"
#include "EntitySystems/Systems.hpp"
#include "update.hpp"
#include "Debug.hpp"
#include "Items.hpp"
#include "GameSave/main.hpp"
#include "sdl.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

// load assets and stuff
int load(SDL_Renderer *ren, float renderScale = 1.0f) {
    int code = 0;
    code |= Textures.load(ren);
    return code;
}

// unload assets and stuff
void unload() {
    Textures.unload();
}

void setDebugSettings(DebugClass& debug) {
    // shorten name for easier typing while keeping default settings
    DebugSettings& ds = debug.settings;

    ds.drawChunkBorders = false;
    ds.drawChunkCoordinates = false;
    ds.drawEntityRects = false;
    ds.drawEntityIDs = false;
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
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strncpy(systemsList[i], start, size+1);
        systemsList[i][size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_SYSTEMS; i++) {
        Log("ID: %d, System: %s.", i, systemsList[i]);
    }
}

void logEntityComponentInfo() {
    const char* componentsStr = TOSTRING((COMPONENT_NAMES));
    int length = strlen(componentsStr);
    char componentsStr2[length];
    strcpy(componentsStr2, componentsStr+1);
    componentsStr2[length-2] = '\0';

    char* buffer = (char*)malloc(sizeof(char) * 256 * NUM_COMPONENTS);

    char* componentsList;
    componentsList = buffer;
    int charIndex = 0;
    for (int i = 0; i < NUM_COMPONENTS; i++) {
        const char* start = &componentsStr2[charIndex];
        const char* chr = start;
        int size = 0;
        while (true) {
            if (*chr == ',') {
                break;
            }
            if (*chr == '\0') {
                // Log("failed to find comma");
                break;
            }
            chr++;
            size++;
        }
        strlcpy(&componentsList[i * 256], start, size+1);
        componentsList[i * 256 + size] = '\0';
        charIndex += (size + 2);
    }

    for (int i = 0; i < NUM_COMPONENTS; i++) {
        Log("ID: %d, Component: %s.", i, &componentsList[i * 256]);
        componentNames[i] = &componentsList[i * 256];
    }
}

void initLogging() {
    Log.init("save/log.txt");
    SDL_LogSetOutputFunction(Logger::logOutputFunction, &Log);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
    SDL_LogSetPriority(LOG_CATEGORY_MAIN, SDL_LOG_PRIORITY_INFO);
}

void initPaths() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    pid_t pid = getpid();
    int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    if (ret <= 0) {
        fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
        fprintf(stderr, "    %s\n", strerror(errno));
        Log.Error("Failed to get pid path!");
    } else {
        printf("proc %d: %s\n", pid, pathbuf);

        int pathlen = strlen(pathbuf);

        char pathToTop[512];
        size_t pathToTopSize = pathlen - sizeof("build/faketorio");
        memcpy(pathToTop, pathbuf, pathToTopSize);
        pathToTop[pathToTopSize] = '\0';
        Log.Info("path to top: %s", pathToTop);
        strcpy(assetsPath, str_add(pathToTop, "/assets/"));
        strcpy(shadersPath, str_add(pathToTop, "/src/Rendering/shaders/"));
        Log.Info("assets path: %s", assetsPath);
    }
}

int main(int argc, char** argv) {
    initLogging();

    initPaths();

    DebugClass debug;
    setDebugSettings(debug);
    Debug = &debug;

    Log.useEscapeCodes = true;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            Log.useEscapeCodes = false;
        }
    }

    SDLContext sdlCtx = initSDL();

    logEntitySystemInfo();
    logEntityComponentInfo();

    int screenWidth,screenHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &screenWidth, &screenHeight);

    //load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

    /*

    RenderContext renderContext(sdlCtx.win, sdlCtx.gl);
    initRenderContext(&renderContext);

    GameViewport gameViewport;

    GameState* state = new GameState();
    
    state->init(NULL, &gameViewport);

    for (int e = 0; e < 20000; e++) {
        Vec2 pos = {(float)randomInt(-200, 200), (float)randomInt(-200, 200)};
        auto tree = Entities::Tree(&state->ecs, pos, {1, 1});
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
    Context context = Context(
        sdlCtx,
        state,
        &gui,
        playerControls,
        &gameViewport,
        worldScale,
        lastUpdateMouseState,
        lastUpdatePlayerTargetPos,
        debug,
        metadata
    );

    setDefaultKeyBindings(context, &playerControls);

    if (metadata.vsyncEnabled) {
        metadata.setTargetFps(60);
    } else {
        metadata.setTargetFps(TARGET_FPS);
    }
    metadata.start();
    NC_SetMainLoop(updateWrapper, &context);
    double secondsElapsed = metadata.end();
    Log("Time elapsed: %.1f", secondsElapsed);

    // GameSave::save(state);

    unload();

    delete state;

    */

    Game* game = new Game(sdlCtx);
    game->init(screenWidth, screenHeight);
    game->start();
    game->quit();
    game->destroy();

    Log.destroy();

    quitSDL(&sdlCtx);

    return 0;
}