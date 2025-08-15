#include "utils/Log.hpp"
#include "utils/Debug.hpp"
#include "GUI/Gui.hpp"
#include "memory/StackAllocate.hpp"

bool Logger::setOutputFile(const char* filename) {
    if (outputFile) {
        fclose(outputFile);
    }
    outputFile = fopen(filename, "w+");
    if (!outputFile) {
        return false;
    }
    return true;
}

std::string str_trim(std::string string, const char* substr) {
    size_t substrLen = strlen(substr);
    size_t pos = string.find(substr);
    if (pos != std::string::npos) {
        // trim recursively until none found
        return str_trim(string.substr(0, pos) + string.substr(pos + substrLen), substr);
    }
    return string; // just do nothing if it's not found
}

std::string conciseSourceFilename(const char* file) {
    return str_trim(str_trim(file, PATH_TO_SOURCE "/"), PATH_TO_INCLUDE "/");
}

// may be run from any thread
void Logger::log(LogCategory category, LogPriority priority, const char *message) const {
    if (priority == LogPriority::Debug && (!Debug || !Debug->debugging)) return;

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

    if (logToStdout) {
        switch (category) {
        using namespace LogCategories;
        default:
            fputs(consoleMessage.data(), stdout);
            break;
        }
    }

    static std::mutex mutex;
    if (gameConsoleOutput || outputFile) {
        // mutating non thread safe stuff, need mutex
        std::lock_guard lock{mutex};
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
}

// does vsnprintf attempting to use a buffer but falls back to malloc if the buffer was not big enough
// returns an allocated ptr to be freed with free, or null if the buffer was used
[[nodiscard]]
char* vsprintf_alloc(char* buffer, size_t size, const char* fmt, va_list ap) {
    int messageLength = vsnprintf(buffer, size, fmt, ap);
    if (messageLength >= size) {
        // couldn't fit entire message onto stack
        char* buffer = (char*)malloc(messageLength + 1);
        messageLength = vsnprintf(buffer, messageLength + 1, fmt, ap);
        return buffer;
    }
    return nullptr;
}

// does snprintf attempting to use a buffer but falls back to malloc if the buffer was not big enough
// returns an allocated ptr to be freed with free, or null if the buffer was used
[[nodiscard]]
char* snprintf_alloc(char* buffer, size_t size, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char* formatted = vsprintf_alloc(buffer, size, fmt, ap);
    va_end(ap);
    return formatted;
}

void Logger::logV(LogCategory category, LogPriority priority, const char* fmt, va_list ap) const {
    char stackBuf[256];
    
    if (char* message = vsprintf_alloc(stackBuf, sizeof(stackBuf), fmt, ap)) {
        this->log(category, priority, message);
        free(message);
    } else {
        this->log(category, priority, stackBuf);
    }
}

Logger gLogger;

void logInternal(LogCategory category, LogPriority priority, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    gLogger.logV(category, priority, fmt, ap);
    va_end(ap);
}

void logInternalWithPrefix(LogCategory category, LogPriority priority, const char* prefix, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    char buffer[128];
    char* allocedMessage = vsprintf_alloc(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    char* message = allocedMessage ? allocedMessage : buffer;

    char withPrefixBuffer[128];
    char* allocedMsgWithPrefix = snprintf_alloc(withPrefixBuffer, sizeof(withPrefixBuffer), "%s%s", prefix, message);
    char* messageWithPrefix = allocedMsgWithPrefix ? allocedMsgWithPrefix : withPrefixBuffer;
        
    gLogger.log(category, priority, messageWithPrefix);

    if (allocedMsgWithPrefix)
        free(allocedMsgWithPrefix);

    if (allocedMessage)
        free(allocedMessage);
}

void logOnceInternal(LogCategory category, LogPriority priority, char* lastMessage, const char* fmt, ...) {
    va_list ap;
    char messageBuf[128];

    va_start(ap, fmt);
    char* allocedMessage = vsprintf_alloc(messageBuf, sizeof(messageBuf), fmt, ap);
    va_end(ap);

    char* message = allocedMessage ? allocedMessage : messageBuf;

    if (strcmp(message, lastMessage) == 0) {
        // same as last, don't print
    } else {
        gLogger.log(category, priority, message);
        strlcpy(lastMessage, message, LOG_ONCE_LAST_MESSAGE_BUF_SIZE);
    }

    if (allocedMessage)
        free(allocedMessage);
}

void crash(CrashReason reason, const char* message) {
    switch (reason) {
    case CrashReason::MemoryFail:
        LogCritical("Memory Error - %s", message);
        break;
    case CrashReason::GameInitialization:
        LogCritical("Failed to initialize game - %s", message);
        break;
    case CrashReason::SDL_Initialization:
        LogCritical("Failed to initalize SDL - %s", message);
        break;
    case CrashReason::UnrecoverableError:
        LogCritical("Unrecoverable Error - %s", message);
        break;
    }
    exit(EXIT_FAILURE);
}

void logCrash(CrashReason reason, const char* fmt, ...) {
    char buf[512];
    
    va_list ap;
    va_start(ap, fmt);
    snprintf(buf, sizeof(buf), fmt, ap);
    gLogger.log(LogCategories::Main, LogPriorities::Crash, buf);
    va_end(ap);
    crash(reason, buf);
}

