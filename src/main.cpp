#include "constants.hpp"
#include "utils/Log.hpp"

#include <vector>
#include <string>
#include <stdio.h>
#include <libproc.h>
#include <unistd.h>

#include "sdl_gl.hpp"

#include "GUI/GUI.hpp"
#include "utils/Debug.hpp"
#include "loadData.hpp"

#include "Game.hpp"
#include "utils/Debug.hpp"
#include "GameSave/main.hpp"
#include "sdl.hpp"
#include "global.hpp"
#include "My/HashMap.hpp"
#include "Rendering/TexturePacker.hpp"

#include "memory.hpp"

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
    SDL_LogSetOutputFunction(Logger::logOutputFunction, &gLogger);
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR);
    SDL_LogSetPriority(LogCategory::Main, SDL_LOG_PRIORITY_INFO);
}

#define LOG LogInfo

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

        FileSystem = FileSystemT(filePrefix);
        LOG("resources path: %s", FileSystem.resources.get());
    }

    LOG("Path to executable: %s\n", pathbuf);
    LOG("Base path: %s\n", basePath);
}

inline void tester() {
    auto map = My::HashMap<int, int>(1);
    
    map.insert(1, 5);
    assert(*map.lookup(1) == 5);
    assert(map.lookup(0) == nullptr);
    assert(map.lookup(3) == nullptr);

    for (int i = 0; i < 100; i++) {
        map.insert(i, 100 - i);
    }

    map.destroy();
    map = My::HashMap<int, int>(342);
    for (int i = 0; i < 10000; i++) {
        map.insert(rand(), rand());
    }

    for (int i = 0; i < 10000; i++) {
        map.remove(rand());
    }

    for (int i = 0; i < 10000; i++) {
        map.lookup(rand());
    }

    for (int i = 0; i < 10000; i++) {
        map.insert(rand(), rand());
    }

    TexturePacker packer({512, 512});
    std::vector<unsigned char> zeroes(512*512);
    packer.packTexture(&zeroes[0], {64, 64});
    packer.packTexture(&zeroes[0], {64, 64});
    packer.packTexture(&zeroes[0], {32, 32});
    packer.packTexture(&zeroes[0], {128, 16});
    packer.packTexture(&zeroes[0], {4, 8});
    packer.packTexture(&zeroes[0], {512, 512});

}

int main(int argc, char** argv) {
    gLogger.useEscapeCodes = true;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            gLogger.useEscapeCodes = false;
        }
    }

    initLogging();
    initPaths();
    gLogger.init(FileSystem.save.get("log.txt"));

    LogInfo("argv[0]: %s\n", argv[0]);

    SDLContext sdlCtx = initSDL();

    logEntityComponentInfo();

    int screenWidth,screenHeight;
    SDL_GL_GetDrawableSize(sdlCtx.win, &screenWidth, &screenHeight);

    tester();

    //load(sdlCtx.ren, sdlCtx.scale);
    loadTileData();
    loadItemData();

    Game* game = new Game(sdlCtx);
    memory_init([game](size_t size) -> void* {
        auto state = game->state;
        state->ecs.reduceMemoryUsage(1);
        return nullptr;
    });

    game->init(screenWidth, screenHeight);
    game->start(); // start indefinitely long game loop
    // runs after the game loop has ended
    game->quit();
    game->destroy();

    gLogger.destroy();
    FileSystem.destroy();

    quitSDL(&sdlCtx);

    return 0;
}