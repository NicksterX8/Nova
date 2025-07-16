#include "constants.hpp"
#include "utils/Log.hpp"

#include <stdio.h>
#include <libproc.h>
#include <unistd.h>

#include "sdl_gl.hpp"

#include "GUI/Gui.hpp"
#include "Game.hpp"
#include "utils/Debug.hpp"
#include "GameSave/main.hpp"
#include "global.hpp"
#include "utils/system/sysinfo.hpp"

#include "memory/memory.hpp"
#include "physics/physics.hpp"

#include "llvm/PointerUnion.h"
#include "ADT/ValPtrUnion.hpp"
#include "ADT/VecTuple.hpp"
#include "ADT/SmallVector.hpp"
#include "memory/allocators.hpp"

#include "utils/system/signpost.hpp"
#include "world/systems/testNewJobs.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

void initLogging(GUI::Console* console) {
    gLogger.gameConsoleOutput = console;
    SDL_SetLogOutputFunction(Logger::logOutputFunction, &gLogger);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_ERROR);
    SDL_SetLogPriority(LogCategory::Main, SDL_LOG_PRIORITY_INFO);
}

void initPaths() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    const char* basePath = SDL_GetBasePath();

    pid_t pid = getpid();
    int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    if (ret <= 0) {
        fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
        fprintf(stderr, "    %s\n", strerror(errno));
        printf("Error: Failed to get pid path!\n");
    } else {

        char upperPath[512]; strcpy(upperPath, pathbuf);
        upperPath[std::string(pathbuf).find_last_of('/')+1] = '\0';

        FileSystem = FileSystemT(upperPath);
        LogInfo("resources path: %s", FileSystem.resources.get());
    }

    LogInfo("Path to executable: %s\n", pathbuf);
    LogInfo("Base path: %s\n", basePath);
}

void tests() {
    physics_test();

    
}

int main(int argc, char** argv) { 
    std::string windowTitle = "Faketorio";

    gLogger.useEscapeCodes = true;
    
    for (int i = 1; i < argc; i++) {
        printf("argv %d: %s\n", i, argv[i]);
        if (strcmp(argv[i], "--no-color-codes") == 0) {
            gLogger.useEscapeCodes = false;
        }
        if (strcmp(argv[i], "--vscode-lldb") == 0) {
            windowTitle += " - Debugger on";
        }
    }

    Game game;
    game.essentialAllocator.init(1ULL << 15);
    trackAllocator("Essential Allocator", &game.essentialAllocator);
    trackAllocator("Medium allocator", &game.blockAllocator);

    game.metadata = MetadataTracker(TARGET_FPS, TICKS_PER_SECOND, ENABLE_VSYNC);
    Metadata = &game.metadata;
    game.metadata.start(); // have to call this so logging knows what time it is

    initLogging(&game.gui.console);
    initPaths();
    gLogger.init(FileSystem.save.get("log.txt"));
    
    SDL_Rect windowRect = {
        0,
        0,
        1000,
        9999
    };
    SDLContext sdlCtx = initSDL(windowTitle.c_str(), windowRect);
    assert(CACHE_LINE_SIZE == SDL_GetCPUCacheLineSize() && "Incorrect cache line size constant!");
    
    if (Global.multithreadingEnabled) {
        int numCores = SDL_GetNumLogicalCPUCores();
        int numThreadsToPool = MIN(numCores - 1, 3);
        Global.threadManager = ThreadManager(numThreadsToPool);
    }

    tests();

    int code = 0;
    code = game.init(sdlCtx);
    if (code) {
        LogCrash(CrashReason::GameInitialization, "Couldn't init game. code: %d", code);
    }
    code = game.start(); // start indefinitely long game loop
    if (code) {
        LogCrash(CrashReason::GameInitialization, "Couldn't start game. code: %d", code);
    }
    // runs after the game loop has ended
    game.quit();
    game.destroy();

    gLogger.destroy();
    FileSystem.destroy();

    quitSDL(&sdlCtx);

    return 0;
}