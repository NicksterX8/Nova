#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "GUI/Gui.hpp"
#include "memory/StackAllocate.hpp"

std::string str_trim(std::string string, const char* substr) {
    size_t substrLen = strlen(substr);
    size_t pos = string.find(substr);
    if (pos != std::string::npos) {
        return string.substr(0, pos) + string.substr(pos + substrLen);
    }
    return string; // just do nothing if it's not found
}

std::string conciseSourceFilename(const char* file) {
    return str_trim(str_trim(file, PATH_TO_SOURCE "/"), PATH_TO_INCLUDE "/");
}

void Logger::logOutputFunction(LogCategory category, LogPriority priority, const char *message) const {
    if (priority == LogPriority::Debug) {
        if (!Debug->debugging) {
            return;
        }
    }

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

    StackAllocate<char, 256> text{messageLength + 64};
    snprintf(text, messageLength+64, "%s%s", prefix, message);

    StackAllocate<char, 256> consoleMessage{messageLength + 64};
    if (useEscapeCodes)
        snprintf(consoleMessage, messageLength + 64, "\033[%s;0;%sm%s\033[0m%c", effect, fg, text.data(), end);
    else
        snprintf(consoleMessage, messageLength + 64, "%s%c", text.data(), end);

    switch (category) {
    using namespace LogCategories;
    case Main:
        printf("%s", consoleMessage.data());
        break;
    case Test:
        printf("test message\n");
        break;
    default:
        printf("%s", consoleMessage.data());
        break;
    }

    if (outputFile) {
        StackAllocate<char, 128> fileMessage{messageLength+1};
        strncpy(fileMessage, message, messageLength);
        fileMessage[messageLength] = '\n';
        fwrite(fileMessage, messageLength+1, 1, outputFile);
    }

    if (gameConsoleOutput) {
        GUI::Console::MessageType messageType = GUI::Console::MessageType::Default;
        if (priority == LogPriorities::Error)
            messageType = GUI::Console::MessageType::Error;
        // remove full file paths from console output cause its ugly and unnecessary
        auto conciseMessage = conciseSourceFilename(message);
        gameConsoleOutput->newMessage(conciseMessage.c_str(), messageType);
    }
}

void Logger::logOutputFunction(void* arg, int category, SDL_LogPriority priority, const char *message) {
    Logger* logger = static_cast<Logger*>(arg);
    logger->logOutputFunction((LogCategory)category, (LogPriority)priority, message);
}

Logger gLogger;

void logInternal(LogCategory category, LogPriority priority, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    SDL_LogMessageV((int)category, (SDL_LogPriority)priority, fmt, ap);
    va_end(ap);
}

void logInternalWithPrefix(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, MAX_LOG_MESSAGE_LENGTH, fmt, ap);
    // Format: FILE:LINE:FUNCTION - MESSAGE
    // The FILE:LINE format is useful because in VSCode you can command click on it to bring you to the file.
    SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s%s", prefix, message);
    va_end(ap);
}

void logOnceInternal(LogCategory category, LogPriority priority, char* lastMessage, const char* fmt, ...) {
    va_list ap;
    char message[MAX_LOG_MESSAGE_LENGTH];

    va_start(ap, fmt);
    vsnprintf(message, 256, fmt, ap);

    if (strcmp(message, lastMessage) == 0) {
        // same as last, don't print
    } else {
        SDL_LogMessage((int)category, (SDL_LogPriority)priority, "%s", message);
        strcpy(lastMessage, message);
    }
    va_end(ap);
}

