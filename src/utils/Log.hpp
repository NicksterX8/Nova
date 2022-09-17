#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <SDL2/SDL_log.h>
#include <stdio.h>

namespace LogCategories {
enum LogCategory {
    Main = SDL_LOG_CATEGORY_CUSTOM,
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
    NumPriorities
};
}

using LogPriority = LogPriorities::LogPriority;

class Logger {
    void logOutputFunction(LogCategory category, LogPriority priority, const char *message) const;
public:
    LogCategory category = LogCategory::Main; // The log category to use when logging, default is main
    bool useEscapeCodes = false; // Use ansi escape codes for colors and bolding when logging to console
    bool logToConsole = true;
    char logOutputFilepath[512] = {'\0', };
    FILE* outputFile = NULL;
    static void logOutputFunction(void* logger, int category, SDL_LogPriority priority, const char *message);

    int init(const char* outputFilepath) {
        if (!outputFilepath)
            return -1;
        strcpy(logOutputFilepath, outputFilepath);
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

extern Logger Log;

#define MAX_LOG_MESSAGE_LENGTH 2048

void logAt(const char* file, int line, LogCategory category, LogPriority priority, const char* fmt, ...);

#define LogAt(...) logAt(__FILE__, __LINE__, SDL_LOG_PRIORITY_INFO, __VA_ARGS__)

enum class space {
    Error,
    Debug,
    Info
};

#define LogBase(category, priority, ...) logAt(__FILE__, __LINE__, category, priority, __VA_ARGS__)
#define LogGory(category_enum_name, priority_enum_name, ...) logAt(__FILE__, __LINE__, LogCategory::category_enum_name, LogPriority::priority_enum_name, __VA_ARGS__)
#define Log(priority_enum_name, ...) LogBase(Log.category, LogPriority::priority_enum_name, __VA_ARGS__)
//#define Log(...) LogBase(Log.category, LogPriority::Info, __VA_ARGS__)

#undef LOG_PRIORITY

#endif