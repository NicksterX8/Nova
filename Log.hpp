#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <SDL2/SDL_log.h>
#include <stdio.h>

enum LogCategory {
    LOG_CATEGORY_MAIN = SDL_LOG_CATEGORY_CUSTOM,
    LOG_CATEGORY_TEST,
    LOG_CATEGORY_DEBUG
};

/*

void CustomLog(void *arguments, int category, SDL_LogPriority priority, const char *message);

#define Log(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_INFO, fmt, ##__VA_ARGS__)
#define LogError(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_ERROR, fmt, ##__VA_ARGS__)
#define LogWarn(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_WARN, fmt, ##__VA_ARGS__)
#define LogCritical(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_CRITICAL, fmt, ##__VA_ARGS__)
#define LogMessage(priority, fmt, ...) SDL_LogMessage(LogCategory, priority, fmt, ##__VA_ARGS__)

*/

class Logger {
    void logOutputFunction(int category, SDL_LogPriority priority, const char *message) const;
public:
    int category = LOG_CATEGORY_MAIN;
    bool useEscapeCodes = false;
    static void logOutputFunction(void* logger, int category, SDL_LogPriority priority, const char *message);

    #define LOG_PRIORITY(priority) {va_list ap;\
        va_start(ap, fmt);\
        SDL_LogMessageV(category, priority, fmt, ap);\
        va_end(ap);}

    inline void Default(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_INFO);
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

    inline void operator()(const char* fmt, ...) const {
        LOG_PRIORITY(SDL_LOG_PRIORITY_INFO);
    }

    #undef LOG_PRIORITY
};

extern Logger Log;

#endif