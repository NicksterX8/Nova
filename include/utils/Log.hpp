#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <string>
#include <stdio.h>
#include "common-macros.hpp"

namespace GUI {
    struct Console;
};

namespace LogCategories {
enum LogCategory {
    Main,
    Render,
    Audio,
    Test,
    Debug
};
}

using LogCategory = LogCategories::LogCategory;

namespace LogPriorities {
enum LogPriority {
    Verbose,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Crash,
    NumPriorities
};
}

using LogPriority = LogPriorities::LogPriority;

struct Logger {
    // thread safe
    void log(LogCategory category, LogPriority priority, const char* message) const;
    // log with message to be formatted with va_list
    void logV(LogCategory category, LogPriority priority, const char* fmt, va_list) const;
public:
    bool initialized = false;
    LogCategory category = LogCategory::Main; // The log category to use when logging, default is main
    FILE* outputFile = NULL;
    bool useEscapeCodes = false; // Use ansi escape codes for colors and bolding when logging to console
    bool logToConsole = true;
    // different than terminal
    GUI::Console* gameConsoleOutput;

    // returns true on success, returns false if we were unable to open the file
    bool setOutputFile(const char* filename);

    void destroy() {
        if (outputFile) {
            fclose(outputFile);
            outputFile = NULL;
        }
    }
};

extern Logger gLogger;

#define MAX_LOG_MESSAGE_LENGTH 512

__attribute((format(printf, 3, 4)))
void logInternal(LogCategory category, LogPriority priority, const char* fmt, ...);

__attribute((format(printf, 4, 5)))
void logInternalWithPrefix(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...);

__attribute((format(printf, 4, 5)))
void logOnceInternal(LogCategory category, LogPriority priority, char* lastMessage, const char* fmt, ...);

#define LOG_LOCATION __FILE__ ":" TOSTRING(__LINE__)

#define LogLocBase(category, priority, ...) logInternal(__FILE__ ":" TOSTRING(__LINE__) " - ", category, priority, __VA_ARGS__)
#define LogLocGory(category_enum_name, priority_enum_name, ...) logAt(__FILE__, __LINE__, LogCategory::category_enum_name, LogPriority::priority_enum_name, __VA_ARGS__)
#define LogLoc(priority_enum_name, ...) LogLocBase(Log.category, LogPriority::priority_enum_name, __VA_ARGS__)

#define LogCritical(...) logInternal(gLogger.category, LogPriority::Critical, __VA_ARGS__)
#define LogErrorLoc(...) logInternal(gLogger.category, LogPriority::Error, LOG_LOCATION " - " __VA_ARGS__)
#define LogError(...) logInternal(gLogger.category, LogPriority::Error, __VA_ARGS__)
#define LogWarnLoc(...) logInternal(gLogger.category, LogPriority::Warn, __FILE__ ":" TOSTRING(__LINE__) " - " __VA_ARGS__)
#define LogWarn(...) logInternal(gLogger.category, LogPriority::Warn, __VA_ARGS__)
#define LogInfo(...) logInternal(gLogger.category, LogPriority::Info, __VA_ARGS__)
#define LogDebug(...) logInternal(gLogger.category, LogPriority::Debug, __VA_ARGS__)
#define LogVerbose(...) logInternal(gLogger.category, LogPriority::Verbose, __VA_ARGS__)

#define LOG_ONCE_LAST_MESSAGE_BUF_SIZE 256
#define LogOnce(priority, ...) { thread_local char COMBINE(_lastMessage, __LINE__)[LOG_ONCE_LAST_MESSAGE_BUF_SIZE]; logOnceInternal(gLogger.category, LogPriority::priority, COMBINE(_lastMessage, __LINE__), __FILE__ ":" TOSTRING(__LINE__) " - " __VA_ARGS__); }


enum class CrashReason {
    SDL_Initialization,
    MemoryFail,
    GameInitialization,
    UnrecoverableError
};

void crash(CrashReason reason, const char* message);

#define CRASH(reason, message) crash(reason, message)

__attribute((format(printf, 2, 3)))
void logCrash(CrashReason reason, const char* fmt, ...);

#define LogCrash(reason, ...) logCrash(reason, __VA_ARGS__)

#endif