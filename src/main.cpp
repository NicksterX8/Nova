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
#include "utils/systemInfo.hpp"

#include "memory.hpp"
#include "physics/physics.hpp"

#include "llvm/PointerUnion.h"
#include "My/ValPtrUnion.hpp"
#include "My/VecTuple.hpp"
#include "My/VecWithAllocator.hpp"
#include "utils/allocators.hpp"

#ifdef DEBUG
    //#include "Testing.hpp"
#endif

void initLogging() {
    SDL_SetLogOutputFunction(Logger::logOutputFunction, &gLogger);
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_ERROR);
    SDL_SetLogPriority(LogCategory::Main, SDL_LOG_PRIORITY_INFO);
}

#define LOG LogInfo

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
        LOG("resources path: %s", FileSystem.resources.get());
    }

    LOG("Path to executable: %s\n", pathbuf);
    LOG("Base path: %s\n", basePath);
}

void tests() {
    physics_test();

    TinyValVector<uint16_t> tinyVec;

    tinyVec.push_back(7);

    auto* data = tinyVec.data();
    LogInfo("val: %d", (int)data[0]);
}

struct Big {
    int64_t a;
    double b;
};

void testy() {
    Vec2 p = {3.2f, -2.1};
    // auto u = ValPtrUnionFull<Vec2, Vec2*, ReducedPrecisionXVec2Info>(p);

    // uintptr_t testPointerVal = (uintptr_t)1 << 31;
    // Vec2* testPtr = (Vec2*)testPointerVal;
    
    // auto p2 = u.getValue();
    // LogInfo("p2: %f, %f", p2.x, p2.y);
    // LogInfo("is pointer? : %d", u.isPointer());
    // auto next = u.getValue();

    // Vec2 p3{-20, 5};
    // u = &p3;
    // Vec2* pointer = u.getPointer();

    // pointer->x = 5.0f;

    // TinyValVectorFull<Big, BigInfo> tinyVec;
    // tinyVec.push_back({8008135, 2.345678910});

    TinyValVector<int, 32> tinyIntVec;
    tinyIntVec.push_back(5);
    auto i2 = tinyIntVec[0];
    tinyIntVec.push_back(2);

    constexpr uint8_t FirstVecXMantissaBit = 0;

    TinyValVector<Vec2, FirstVecXMantissaBit> tinyVecVec;

    auto i = tinyIntVec[1];
    tinyIntVec.reserve(5);
    tinyIntVec.resize(2, 12);
    LogInfo("size : %d", tinyIntVec.size());

    TinyValVector<Big, 0, true, true> bigs;

    bigs.push_back({200001, 1.2345678910111213});
    bigs.push_back({200011, -0.5});

    auto big = bigs[0];

    auto big2 = bigs[1];


    VecTuple<int, float> vecTuple{&mallocator};
    vecTuple.push_back(2, 3.0f);
    float far[] = {4.0f, 2.3f};
    int iar[] = {1, 2};
    vecTuple.append(2, iar, far);

    auto f = vecTuple.get<float>(0);

    auto tuple = vecTuple[0];
    int x = vecTuple[1].get<int>();

    vecTuple.resize(5);

    ScratchAllocator<> scratch(10000);
    My::VecWithAllocator<int, ScratchAllocator<>> vecWithAlloc(std::move(scratch));
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

    initLogging();
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
    LogError("Function: %s", __FUNCTION__);

    testy();

    Game* game = new Game(sdlCtx);
    Mem::init(
        [&]() { // need memory
            //auto state = game->state;
            //state->ecs.minmizeMemoryUsage();
        },
        [&]() { // failed to get enough memory, crash.
            logCrash(CrashReason::MemoryFail, "Out of memory!");
        }
    );

    int code = 0;
    code = game->init();
    if (code) {
        LogCrash(CrashReason::GameInitialization, "Couldn't init game. code: %d", code);
    }
    code = game->start(); // start indefinitely long game loop
    if (code) {
        LogCrash(CrashReason::GameInitialization, "Couldn't start game. code: %d", code);
    }
    // runs after the game loop has ended
    game->quit();
    game->destroy();

    gLogger.destroy();
    FileSystem.destroy();

    quitSDL(&sdlCtx);

    return 0;
}