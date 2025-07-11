#ifndef COMMANDS_INCLUDED
#define COMMANDS_INCLUDED

#include <vector>
#include <string>
#include <functional>
#include "utils/Log.hpp"
#include "ADT/ArrayRef.hpp"

struct Game;

struct Command {
    char name[32];
    std::string description;
    int nameLength;
    struct Result {
        enum Type {
            Error,
            Warn,
            Success,
            Other,
            Null
        } type = Null;

        std::string message;

    };
    using FunctionType = std::function<Result(const char* arguments)>;
    FunctionType function;

    static Command make(const char* name, const FunctionType& function) {
        Command command;
        int nameLength = (int)strlen(name);
        if (nameLength < 32) {
            memcpy(command.name, name, nameLength+1);
        } else {
            LogError("Command name passed was longer than allowed! name: %s. max length: %d", name, 32);
            command.name[0] = '\0';
            nameLength = 0;
        }
        command.nameLength = nameLength;
        command.function = function;
        return command;
    }
};

struct CommandInput {
    std::string name;
    std::string arguments;

    operator bool() const {
        return !name.empty();
    }
};

extern std::vector<Command> gCommands;

void setCommands(Game* game);

inline Command::Result executeCommand(Command* command, const char* arguments) {
    return command->function(arguments);
}

CommandInput processMessage(std::string message, ArrayRef<Command> possibleCommands);

// returns null if a command can't be found with that name
inline Command* getCommand(std::string name) {
    for (auto& command : gCommands) {
        if (command.name == name) {
            return &command;
        }
    }
    return nullptr;
}

#endif