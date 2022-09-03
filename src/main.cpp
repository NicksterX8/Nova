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
    Log.init(str_add(FilePaths::save, "log.txt"));
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
        printf("Error: Failed to get pid path!\n");
    } else {
        int pathlen = strlen(pathbuf);

        char pathToTop[512];
        size_t pathToTopSize = pathlen - sizeof("build/faketorio");
        memcpy(pathToTop, pathbuf, pathToTopSize);
        pathToTop[pathToTopSize] = '\0';
        printf("path to top: %s\n", pathToTop);
        strcpy(FilePaths::assets, str_add(pathToTop, "/assets/"));
        strcpy(FilePaths::shaders, str_add(pathToTop, "/src/Rendering/shaders/"));
        strcpy(FilePaths::save, str_add(pathToTop, "/save/"));
    }
}

int main(int argc, char** argv) {
    initPaths();
    initLogging();

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

    Game* game = new Game(sdlCtx);
    game->init(screenWidth, screenHeight);
    game->start();
    game->quit();
    game->destroy();

    Log.destroy();

    quitSDL(&sdlCtx);

    return 0;
}