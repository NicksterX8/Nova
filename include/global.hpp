#ifndef GLOBAL_INCLUDED
#define GLOBAL_INCLUDED

#include "constants.hpp"
#include "utils/Log.hpp"

enum class CrashReason {
    MemoryFail
};

inline void crash(CrashReason reason, const char* message) {
    LogCritical("CRASHING: %s", message);
    switch (reason) {
    case CrashReason::MemoryFail:
        LogCritical("Memory Error");
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