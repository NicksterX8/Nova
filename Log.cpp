#include "Log.hpp"

int LogCategory = LOG_CATEGORY_MAIN;

void CustomLog(void *userdata, int category, SDL_LogPriority priority, const char *message) {
    LogArguments args = *static_cast<LogArguments*>(userdata);
    
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

    int messageLength = strlen(message);

    char text[messageLength + 128];
    sprintf(text, "%s%s", prefix, message);

    char consoleMessage[messageLength + 128];
    if (args.useColorCodes)
        sprintf(consoleMessage, "\033[%s;0;%sm%s\033[0m\n", effect, fg, text);
    else
        sprintf(consoleMessage, "%s\n", text);

    switch (category) {
    case LOG_CATEGORY_MAIN:
        printf("%s", consoleMessage);
        break;
    case LOG_CATEGORY_TEST:
        //printf("test message");
        break;
    default:
        printf("%s", consoleMessage);
        break;
    }
}