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
#include "memory/FreelistAllocator.hpp"

void initLogging(GUI::Console* console) {
    gLogger.setOutputFile(FileSystem.save.get("log.txt"));
    gLogger.gameConsoleOutput = console;
}

void initPaths() {
    char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

    // not sure if i should use this instead but everything seems to work fine so far so if it aint broke dont fix it
    // const char* basePath = SDL_GetBasePath();

    pid_t pid = getpid();
    int ret = proc_pidpath(pid, pathbuf, sizeof(pathbuf));
    if (ret <= 0) {
        fprintf(stderr, "PID %d: proc_pidpath ();\n", pid);
        fprintf(stderr, "    %s\n", strerror(errno));
        printf("Error: Failed to get pid path!\n");
    } else {
        char upperPath[512]; strlcpy(upperPath, pathbuf, 512);
        upperPath[std::string(pathbuf).find_last_of('/')+1] = '\0';

        FileSystem = FileSystemT(upperPath);
        printf("resources path: %s", FileSystem.resources.get());
    }

    printf("Path to executable: %s\n", pathbuf);
}

int Main(int argc, char** argv) { 
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

    initPaths();

    Game game;

    initLogging(&game.gui.console);


    game.essentialAllocator.init(1ULL << 13);
    trackAllocator("Essential Allocator", &game.essentialAllocator);
    trackAllocator("Medium allocator", &game.blockAllocator);

    game.metadata = MetadataTracker(TARGET_FPS, TICKS_PER_SECOND, ENABLE_VSYNC);
    Metadata = &game.metadata;
    game.metadata.start(); // have to call this so logging knows what time it is

#ifndef NDEBUG
    LogInfo("Debug build");
#else
    LogInfo("Non debug build");
#endif
    
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