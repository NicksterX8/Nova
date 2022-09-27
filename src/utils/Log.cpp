#include "Log.hpp"

void Logger::logOutputFunction(LogCategory category, LogPriority priority, const char *message) const {
    const char* fg = "";
    const char* prefix = "";
    char end = '\n';
    const char* effect = "";
    switch (priority) {
    using namespace LogPriorities;
    case Debug:
        fg = "94";
        break;
    case LogPriority::Info:
        fg = "";
        break;
    case LogPriority::Warn:
        prefix = "Warning: ";
        fg = "33";
        break;
    case LogPriority::Error:
        prefix = "Error: ";
        fg = "31";
        effect = "1";
        break;
    case LogPriority::Critical:
        prefix = "Critical: ";
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
    if (useEscapeCodes)
        sprintf(consoleMessage, "\033[%s;0;%sm%s\033[0m%c", effect, fg, text, end);
    else
        sprintf(consoleMessage, "%s%c", text, end);

    switch (category) {
    using namespace LogCategories;
    case Main:
        printf("%s", consoleMessage);
        break;
    case Test:
        printf("test message\n");
        break;
    default:
        printf("%s", consoleMessage);
        break;
    }

    if (outputFile) {
        char fileMessage[messageLength+1];
        strncpy(fileMessage, message, messageLength);
        fileMessage[messageLength] = '\n';
        fwrite(fileMessage, messageLength+1, 1, outputFile);
    }
}

void Logger::logOutputFunction(void* arg, int category, SDL_LogPriority priority, const char *message) {
    Logger* logger = static_cast<Logger*>(arg);
    logger->logOutputFunction((LogCategory)category, (LogPriority)priority, message);
}

Logger gLogger;

void logAt(const char* file, int line, LogCategory category, LogPriority priority, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s:%d - %s", file, line, message);
    va_end(ap);
}

void logInternal(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s", prefix, message);
    va_end(ap);
}

void logInternal2(LogCategory category, LogPriority priority, const char* prefix1, const char* prefix2, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s%s", prefix1, prefix2, message);
    va_end(ap);
}

void logInternal3(LogCategory category, LogPriority priority, const char* prefix1, const char* prefix2, const char* prefix3, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s%s%s", prefix1, prefix2, prefix3, message);
    va_end(ap);
}

// FILE:FUNCTION