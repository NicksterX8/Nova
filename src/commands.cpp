#include "commands.hpp"
#include "Game.hpp"
#include "rendering/textures.hpp"
#include "utils/FileSystem.hpp"

namespace Commands {
    using Result = Command::Result;

    // not very useful tbh
    #define DEF_COMMAND(name, ...) Result name(const char* args, __VA_ARGS__)

    class ArgsList {
        char* buffer;
        int bufferSize;
        int charIndex;
        int argc;
    public:
        explicit ArgsList(const char* rawArgs) {
            charIndex = 0;

            int argsLen = strlen(rawArgs);
            if (argsLen > 0) {
                bufferSize = argsLen + 1;
                buffer = Alloc<char>(argsLen + 1);
                memcpy(buffer, rawArgs, argsLen + 1);

                argc = 1;
                for (int i = 0; i < argsLen; i++) {
                    char& c = buffer[i];
                    if (c == ' ') {
                        c = '\0';
                        argc++;
                    }
                }
            } else {
                bufferSize = 0;
                buffer = nullptr;
                argc = 0;
            }
        }

        // get the next argument, and forward them so next time you'll get the one after that
        const char* get() {
            const char* arg;
            if (charIndex+1 < bufferSize) {
                 arg = &buffer[charIndex];
            } else {
                return nullptr;
            }

            // now, forward for next time
            // cond: make sure there are atleast two more characters after nul - atleast one regular character and one terminator
            for (; charIndex < bufferSize; charIndex++) {
                if (buffer[charIndex] == '\0') {
                    charIndex++; // right after the nul
                    break;
                }
            }
            
            return arg;
        }

    };

    using Args = ArgsList;

    Result setMode(Args args, Game* game) {
        const char* mode = args.get();

        game->setMode(modeID(mode));
        char message[512];
        snprintf(message, 512, "Set mode to %s", mode);
        return {message};
    }

    Result setTexture(Args args, TextureManager* textures, TextureArray* textureArray) {
        const char* textureName        = args.get();
        const char* newTextureFilename = args.get();

        char message[512];

        if (!textureName) {
            return {"Failed to set new texture, no texture named."};
        } else if (!newTextureFilename) {
            return {"No new texture given!"};
        }

        for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
            TextureMetaData metadata = textures->metadata[id];
            if (My::streq(metadata.identifier, textureName)) {
                SDL_Surface* surface = IMG_Load(FileSystem.assets.get(newTextureFilename));
                if (!surface) {
                    snprintf(message, 512, "Couldn't load texture with path %s!", (char*)FileSystem.assets.get(newTextureFilename));
                    return {std::string(message)};
                }
                int code = updateTextureArray(textureArray, textures, id, surface);
                if (code) {
                    return {"Failed while setting texture!"};
                }
                return {"Updated texture"};
            }
        }
        
        snprintf(message, 512, "Couldn't find texture with name \"%s\"", textureName);
        return {std::string(message)};
    }

    Result kill(Args args, GameState* state) {
        const char* target = args.get();

        int numDestroyed = 0;
        state->ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::EntityTypeEC> > >(
        [&](Entity entity){
            auto type = state->ecs.Get<const EC::EntityTypeEC>(entity);
            // super inefficient btw
            if (My::streq(target, "ALL") || My::streq(target, type->name)) {
                if (state->ecs.EntityExists(entity)) {
                    state->ecs.Destroy(entity);
                    numDestroyed += 1;
                } else {
                    LogWarn("Entity didn't exist while killing entities");
                }
            }
        });
        char message[512];
        snprintf(message, 512, "Killed %d %ss", numDestroyed, target);
        
        return {std::string(message)};
    } 
}

std::vector<Command> gCommands;

void setCommands(Game* game) {
    using namespace Commands;

    #define REG_COMMAND(name, ...) gCommands.push_back(Command::make(TOSTRING(name), [=](const char* args)->Result{ return name(ArgsList(args), __VA_ARGS__); }))

    auto* state = game->state;
    auto* textures = &game->renderContext->textures;
    auto* textureArray = &game->renderContext->textureArray;
    
    REG_COMMAND(kill, state);
    REG_COMMAND(setMode, game);
    REG_COMMAND(setTexture, textures, textureArray);
}

CommandInput processMessage(std::string message, ArrayRef<Command> possibleCommands) {
    if (message.empty()) {
        return {};
    }
    CommandInput input = {};
    // '/' is for commands, active message is a command
    if (message[0] == '/') {
        const char* enteredCommand = message.c_str() + 1;
        int enteredCommandLength = 0;
        {
            int i = 0;
            while (enteredCommand[i] != '\0' && enteredCommand[i] != ' ') {
                i++;
            }
            enteredCommandLength = i;
        }

        const char* arguments = enteredCommand + enteredCommandLength + 1;
        // if there's no characters after the entered command then...
        if (arguments > &message.back()) {
            arguments = nullptr; // no arguments were provided
        }

        input = CommandInput{std::string(enteredCommand, enteredCommandLength), arguments ? std::string(arguments) : std::string{}};
    }
    return input;
}