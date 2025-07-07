#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#include "constants.hpp"
#include "utils/Log.hpp"
#include "My/String.hpp"
#include "utils/vectors_and_rects.hpp"
#include "threads.hpp"
#include "utils/GlobalAllocators.hpp"

struct GlobalsType {
    unsigned int debugTexture = 0;
    glm::vec2 playerMovement = {NAN, NAN};
    bool mouseClickedOnGui = false;

    bool paused = false;

    bool multithreadingEnabled = USE_MULTITHREADING;
    ThreadManager threadManager;

    GlobalsType() {
        
    }
};

extern GlobalsType Global;

enum Mode : int {
    NullMode,
    Unstarted,
    Paused,
    Playing,
    Quit,
    Testing,
    NumModes
};
static constexpr Mode FirstMode = Unstarted;

inline const char* modeName(Mode mode) {
    switch (mode) {
    case Unstarted:
        return "unstarted";
    case Paused:
        return "paused";
    case Playing:
        return "playing";
    case Quit:
        return "quit";
    case Testing:
        return "testing";
    default:
        break;
    }
    return "null";
}

inline Mode modeID(const char* mode) {
    for (int i = NullMode; i < NumModes; i++) {
        if (My::streq(modeName((Mode)i), mode)) {
            return (Mode)i;
        }
    }
    return NullMode;
}


enum class CrashReason {
    SDL_Initialization,
    MemoryFail,
    GameInitialization,
    UnrecoverableError
};

inline void crash(CrashReason reason, const char* message) {
    switch (reason) {
    case CrashReason::MemoryFail:
        LogCritical("Memory Error - %s", message);
        break;
    case CrashReason::GameInitialization:
        LogCritical("Failed to initialize game - %s", message);
        break;
    case CrashReason::SDL_Initialization:
        LogCritical("Failed to initalize SDL - %s", message);
        break;
    case CrashReason::UnrecoverableError:
        LogCritical("Unrecoverable Error - %s", message);
        break;
    }
    exit(EXIT_FAILURE);
}

#define CRASH(reason, message) crash(reason, message)

inline void logCrash(CrashReason reason, const char* fmt, ...) {
    va_list ap; 
    va_start(ap, fmt);
    char message[MAX_LOG_MESSAGE_LENGTH];
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)LogCategory::Main, (SDL_LogPriority)LogPriority::Crash, "%s%s", "CRASH: ", message);
    crash(reason, message);
}

#define LogCrash(reason, ...) logCrash(reason, __VA_ARGS__)

#define UNFINISHED_CRASH() assert(0 && "Unfinished code, should not be run!")

#endif