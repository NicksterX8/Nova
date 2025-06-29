#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "GUI/Gui.hpp"

void Logger::logOutputFunction(LogCategory category, LogPriority priority, const char *message) const {
    const char* fg = "";
    const char* prefix = "";
    char end = '\n';
    const char* effect = "";
    switch (priority) {
    case LogPriority::Debug:
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

    char* text = Alloc<char>(messageLength + 128);
    snprintf(text, messageLength+128, "%s%s", prefix, message);

    char* consoleMessage = Alloc<char>(messageLength + 128);
    if (useEscapeCodes)
        snprintf(consoleMessage, messageLength + 128, "\033[%s;0;%sm%s\033[0m%c", effect, fg, text, end);
    else
        snprintf(consoleMessage, messageLength + 128, "%s%c", text, end);

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
        char* fileMessage = Alloc<char>(messageLength+1);
        strncpy(fileMessage, message, messageLength);
        fileMessage[messageLength] = '\n';
        fwrite(fileMessage, messageLength+1, 1, outputFile);
    }

    if (Debug && Debug->console) {
        if (true)
            Debug->console->newMessage(message, GUI::Console::MessageType::Error);
    }
}

void Logger::logOutputFunction(void* arg, int category, SDL_LogPriority priority, const char *message) {
    Logger* logger = static_cast<Logger*>(arg);
    logger->logOutputFunction((LogCategory)category, (LogPriority)priority, message);
}

Logger gLogger;

void logAt(const char* file, int line, LogCategory category, LogPriority priority, const char* fmt, ...) {
    if (!gLogger.initialized) {
        printf("Logger not initalized yet!");
        return;
    }
    
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s:%d - %s", file, line, message);
    va_end(ap);
}

void logInternal(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...) {
    if (!gLogger.initialized) {
        printf("Logger not initalized yet!");
        return;
    }
    
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s", prefix, message);
    va_end(ap);
}

void logInternal2(LogCategory category, LogPriority priority, const char* prefix1, const char* prefix2, const char* fmt, ...) {
    if (!gLogger.initialized) {
        printf("Logger not initalized yet!");
        return;
    }
    
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s%s", prefix1, prefix2, message);
    va_end(ap);
}

void logInternal3(LogCategory category, LogPriority priority, const char* file, const char* function, int line, const char* fmt, ...) {
    if (!gLogger.initialized) {
        printf("Logger not initalized yet!");
        return;
    }
    
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    // Format: FILE:LINE:FUNCTION - MESSAGE
    // The FILE:LINE format is useful because in VSCode you can command click on it to bring you to the file.
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s:%d:%s - %s", file, line, function, message);
    va_end(ap);
}

