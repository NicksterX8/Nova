#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <SDL2/SDL_log.h>
#include <stdio.h>

enum LogCategory {
    LOG_CATEGORY_MAIN = SDL_LOG_CATEGORY_CUSTOM,
    LOG_CATEGORY_TEST,
    LOG_CATEGORY_DEBUG
};

class Logger {
    void logOutputFunction(int category, SDL_LogPriority priority, const char *message) const;
public:
    int category = LOG_CATEGORY_MAIN; // The log category to use when logging, default is main
    bool useEscapeCodes = false; // Use ansi escape codes for colors and bolding when logging to console
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