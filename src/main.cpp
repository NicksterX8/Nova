#include "constants.hpp"
#include "utils/Log.hpp"

#include <vector>
#include <string>
#include <stdio.h>
#include <libproc.h>
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "sdl_gl.hpp"

#include "GUI/GUI.hpp"
#include "utils/Debug.hpp"
#include "loadData.hpp"

#include "Game.hpp"
#include "utils/Debug.hpp"
#include "GameSave/main.hpp"
#include "sdl.hpp"

//#include <glog/logging.h>

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

void logEntitySystemInfo() {
    LogInfo("Number of systems: %d", NUM_SYSTEMS);
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
        LogInfo("ID: %d, System: %s.", i, systemsList[i]);
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
        //Log("ID: %d, Component: %s.", i, &componentsList[i * 256]);
        componentNames[i] = &componentsList[i * 256];
    }
}

void initLogging() {
    gLogger.init(str_add(FilePaths::save, "log.txt"));
    SDL_LogSetOutputFunction(Logger::logOutputFunction, &gLogger);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR);
    SDL_LogSetPriority(LogCategory::Main, SDL_LOG_PRIORITY_INFO);
}

#define LOG(...) printf(__VA_ARGS__)

void initPaths() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    char* basePath = SDL_GetBasePath();

    pid_t pid = getpid();
    int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    if (ret <= 0) {
        fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
        fprintf(stderr, "    %s\n", strerror(errno));
        printf("Error: Failed to get pid path!\n");
    } else {

        char upperPath[512]; strcpy(upperPath, pathbuf);
        upperPath[std::string(pathbuf).find_last_of('/')] = '\0';

        auto filePrefix = str_add(upperPath, "/resources/");
        strcpy(FilePaths::base, filePrefix);
        strcpy(FilePaths::assets, str_add(filePrefix, "assets/"));
        strcpy(FilePaths::shaders, str_add(filePrefix, "shaders/"));
        strcpy(FilePaths::save, str_add(filePrefix, "save/"));

        LOG("Assets path: %s\n", FilePaths::assets);
        LOG("Shaders path: %s\n", FilePaths::shaders);
    }

    LOG("Path to executable: %s\n", pathbuf);
    LOG("Base path: %s\n", basePath);
}

int main(int argc, char** argv) {
    gLogger.useEscapeCodes = true;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            gLogger.useEscapeCodes = false;
        }
    }

    initPaths();
    initLogging();

    LogCritical("(Log(Critcal, ...)): ayo bud. my name is %s and im %d years old\n", "nick", 12);
    LogWarn("(LogWarn,...)): whats up guys");
    LogError("LogError: heyo");

    printf("argv[0]: %s\n", argv[0]);

    SDLContext sdlCtx = initSDL();

    //logEntitySystemInfo();
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

    gLogger.destroy();

    quitSDL(&sdlCtx);

    return 0;
}