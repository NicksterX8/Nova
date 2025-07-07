#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <SDL3/SDL_log.h>
#include <string>
#include <stdio.h>
#include "common-macros.hpp"

namespace LogCategories {
enum LogCategory {
    Main = SDL_LOG_CATEGORY_CUSTOM,
    Render,
    Audio,
    Test,
    Debug
};
}

using LogCategory = LogCategories::LogCategory;

namespace LogPriorities {
enum LogPriority {
    Verbose = SDL_LOG_PRIORITY_VERBOSE,
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
    void logOutputFunction(LogCategory category, LogPriority priority, const char *message) const;
public:
    bool initialized = false;
    LogCategory category = LogCategory::Main; // The log category to use when logging, default is main
    bool useEscapeCodes = false; // Use ansi escape codes for colors and bolding when logging to console
    bool logToConsole = true;
    char logOutputFilepath[512] = {'\0', };
    FILE* outputFile = NULL;

    static void logOutputFunction(void* logger, int category, SDL_LogPriority priority, const char *message);

    int init(const char* outputFilepath) {
        initialized = true;
        if (!outputFilepath)
            return -1;
        strncpy(logOutputFilepath, outputFilepath, 512);
        outputFile = fopen(logOutputFilepath, "w+");
        if (!outputFile)
            return -1;
        return 0;
    }

    void destroy() {
        if (outputFile) {
            fclose(outputFile);
            outputFile = NULL;
        }
    }

    #define LOG_PRIORITY(priority) {va_list ap;\
        va_start(ap, fmt);\
        SDL_LogMessageV(category, priority, fmt, ap);\
        va_end(ap);}
    

    inline void Default(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_INFO);
    }

    inline void Priority(SDL_LogPriority priority, const char* fmt, ...) {
        LOG_PRIORITY(priority);
    }

    inline void Info(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_INFO);
    }

    inline void Error(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_ERROR);
    }

    inline void Warn(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_WARN);
    }

    inline void Critical(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_CRITICAL);
    }

    inline void Message(SDL_LogPriority priority, const char* fmt, ...) const {
        LOG_PRIORITY(priority);
    }

    //inline void operator()(const char* fmt, ...) const {
      //  LOG_PRIORITY(SDL_LOG_PRIORITY_INFO);
    //}
};

extern Logger gLogger;

#define MAX_LOG_MESSAGE_LENGTH 512

void logInternal(LogCategory category, LogPriority priority, const char* fmt, ...);
void logInternalWithPrefix(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...);
void logOnceInternal(LogCategory category, LogPriority priority, char* lastMessage, const char* fmt, ...);

#define _FUNCTION_ __FUNCTION__
#define LOG_LOCATION __FILE__ ":" TOSTRING(__LINE__)

#define LogLocBase(category, priority, ...) logInternal(__FILE__ ":" TOSTRING(__LINE__) " - ", category, priority, __VA_ARGS__)
#define LogLocGory(category_enum_name, priority_enum_name, ...) logAt(__FILE__, __LINE__, LogCategory::category_enum_name, LogPriority::priority_enum_name, __VA_ARGS__)
#define LogLoc(priority_enum_name, ...) LogLocBase(Log.category, LogPriority::priority_enum_name, __VA_ARGS__)

#define LogCritical(...) logInternal(gLogger.category, LogPriority::Critical, __VA_ARGS__)
#define LogError(...) logInternal(gLogger.category, LogPriority::Error, LOG_LOCATION " - " __VA_ARGS__)
#define LogWarn(...) logInternal(gLogger.category, LogPriority::Warn, __FILE__ ":" TOSTRING(__LINE__) " - " __VA_ARGS__)
#define LogInfo(...) logInternal(gLogger.category, LogPriority::Info, __VA_ARGS__)
#define LogDebug(...) logInternal(gLogger.category, LogPriority::Debug, __VA_ARGS__)

#define LogOnce(priority, ...) { thread_local char COMBINE(_lastMessage, __LINE__)[256]; logOnceInternal(gLogger.category, LogPriority::priority, COMBINE(_lastMessage, __LINE__), __FILE__, __LINE__, __VA_ARGS__); }

#undef LOG_PRIORITY

#endif