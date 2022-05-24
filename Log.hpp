#ifndef LOG_INCLUDED
#define LOG_INCLUDED

#include <SDL2/SDL_log.h>
#include <stdio.h>

enum LogCategory {
    LOG_CATEGORY_MAIN = SDL_LOG_CATEGORY_CUSTOM,
    LOG_CATEGORY_TEST,
    LOG_CATEGORY_DEBUG
};

extern int LogCategory;

void CustomLog(void *userdata, int category, SDL_LogPriority priority, const char *message);

#define Log(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_INFO, fmt, ##__VA_ARGS__)
#define LogError(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_ERROR, fmt, ##__VA_ARGS__)
#define LogWarn(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_WARN, fmt, ##__VA_ARGS__)
#define LogCritical(fmt, ...) SDL_LogMessage(LogCategory, SDL_LOG_PRIORITY_CRITICAL, fmt, ##__VA_ARGS__)
#define LogMessage(priority, fmt, ...) SDL_LogMessage(LogCategory, priority, fmt, ##__VA_ARGS__)

#endif