#include "Log.hpp"

int LogCategory = LOG_CATEGORY_MAIN;

void CustomLog(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    const char* fg = "";
    const char* prefix = "";
    const char* effect = "";
    switch (priority) {
    case SDL_LOG_PRIORITY_DEBUG:
        fg = "33";
        break;
    case SDL_LOG_PRIORITY_INFO:
        fg = "";
        break;
    case SDL_LOG_PRIORITY_ERROR:
        prefix = "ERROR: ";
        fg = "31";
        effect = "1";
        break;
    case SDL_LOG_PRIORITY_CRITICAL:
        prefix = "CRITICAL: ";
        fg = "91";
        effect = "1";
        break;
    default:
        break;
    }

    switch (category) {
    case LOG_CATEGORY_MAIN:
        printf("\033[%s;0;%sm%s%s\033[0m\n", effect, fg, prefix, message);
        break;
    case LOG_CATEGORY_TEST:
        //printf("test message");
        break;
    default:
        printf("\033[%s;0;%sm%s%s\033[0m\n", effect, fg, prefix, message);
        break;
    }
}