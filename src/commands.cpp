#include "commands.hpp"
#include "Game.hpp"
#include "rendering/textures.hpp"
#include "utils/FileSystem.hpp"
#include <sstream>

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
            int objectDepth = 0;

            int argsLen = strlen(rawArgs);
            if (argsLen > 0) {
                bufferSize = argsLen + 1;
                buffer = Alloc<char>(argsLen + 1);
                memcpy(buffer, rawArgs, argsLen + 1);

                argc = 1;
                for (int i = 0; i < argsLen; i++) {
                    char& c = buffer[i];
                    if (c == '{') {
                        objectDepth++;
                    } else if (c == '}') {
                        objectDepth--;
                        if (objectDepth < 0) objectDepth = 0;
                    }
                    else if (c == ' ') {
                        if (objectDepth == 0) {
                            c = '\0';
                            argc++;
                        }
                    }
                }
            } else {
                bufferSize = 0;
                buffer = nullptr;
                argc = 0;
            }
        }

        // get the next argument, and forward them so next time you'll get the one after that
        std::string get() {
            const char* arg;
            size_t size = 0;
            if (charIndex+1 < bufferSize) {
                 arg = &buffer[charIndex];
            } else {
                return {};
            }

            // now, forward for next time
            // cond: make sure there are atleast two more characters after nul - atleast one regular character and one terminator
            for (; charIndex < bufferSize; charIndex++) {
                if (buffer[charIndex] == '\0') {
                    charIndex++; // right after the nul
                    break;
                }
                size++;
            }
            
            return std::string(arg, size);
        }

    };

    using Args = ArgsList;

    Result setMode(Args args, Game* game) {
        auto mode = args.get();

        game->setMode(modeID(mode.c_str()));
        char message[512];
        snprintf(message, 512, "Set mode to %s", mode.c_str());
        return {message};
    }

    Result setTexture(Args args, TextureManager* textures, TextureArray* textureArray) {
        auto textureName        = args.get();
        auto newTextureFilename = args.get();

        char message[512];

        if (textureName.empty()) {
            return {"Failed to set new texture, no texture named."};
        } else if (newTextureFilename.empty()) {
            return {"No new texture given!"};
        }

        for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
            TextureMetaData metadata = textures->metadata[id];
            if (My::streq(metadata.identifier, textureName.c_str())) {
                auto filepath = FileSystem.assets.get(newTextureFilename.c_str());
                SDL_Surface* surface = IMG_Load(filepath);
                if (!surface) {
                    snprintf(message, 512, "Couldn't load texture with path %s!", filepath.str);
                    return {std::string(message)};
                }
                int code = updateTextureArray(textureArray, textures, id, surface);
                if (code) {
                    return {"Failed while setting texture!"};
                }
                return {"Updated texture"};
            }
        }
        
        snprintf(message, 512, "Couldn't find texture with name \"%s\"", textureName.c_str());
        return {std::string(message)};
    }

    Result setComponent(Args args, EntityWorld* ecs) {
        auto entityIdStr = args.get();
        auto componentName = args.get();
        auto componentValueStr = args.get();

        if (entityIdStr.empty() || componentName.empty() || componentValueStr.empty()) {
            return {"Not enough arguments provided!"};
        }

        EntityID entityID = atoi(entityIdStr.c_str());
        Entity entity = Entity(entityID, ECS::WILDCARD_ENTITY_VERSION);
        if (!ecs->EntityExists(entity)) {
            return {"That entity doesn't exist!"};
        }

        ComponentID componentID = ecs->GetComponentIdFromName(componentName.c_str());
        if (componentID == NullComponentID) {
            return {"Invalid component!"};
        }

        // str should begin and end in braces, need atleast 2 characters to do that
        //if (componentValueStr.size() < 2) return {"Invalid component value"} ;
        //std::string interiorValue = componentValueStr.substr(1, componentValueStr.size()-2); // cut off braces

        void* component = ecs->Get(componentID, entity);
        if (!component) {
            // entity doesn't have this component, so add it
            ecs->Add(componentID, entity);
        }

        auto componentSize = ecs->getComponentSize(componentID);
        if (componentSize <= 0) return {"Invalid component!"};

        bool success = ecs->GetComponentValueFromStr(componentID, componentValueStr, component);
        if (success) {
            return {"Successfully set component value"};
        }
        return {"Failed to parse component value"};
    }

    Result kill(Args args, GameState* state) {
        auto target = args.get();

        int numDestroyed = 0;
        state->ecs.ForEach< EntityQuery< ECS::RequireComponents<EC::EntityTypeEC> > >(
        [&](Entity entity){
            auto type = state->ecs.Get<const EC::EntityTypeEC>(entity);
            // super inefficient btw
            if (target == "ALL" || target == type->name) {
                if (state->ecs.EntityExists(entity)) {
                    state->ecs.Destroy(entity);
                    numDestroyed += 1;
                } else {
                    LogWarn("Entity didn't exist while killing entities");
                }
            }
        });
        char message[512];
        snprintf(message, 512, "Killed %d %ss", numDestroyed, target.c_str());
        
        return {std::string(message)};
    }

    Result showTexture(Args args, const TextureManager* textures) {
        if (g.debugTexture) {
            glDeleteTextures(1, &g.debugTexture);
        }

        auto texture = args.get();

        TextureID texID = textures->getID(texture.c_str());
        auto& metadata = textures->metadata[texID];

        g.debugTexture = GlLoadSurface(loadSurface(FileSystem.assets.get(metadata.filename)), GL_NEAREST, GL_NEAREST);
        return {"Showing texture"};
    }

    Result reloadShader(Args args, RenderContext* renderContext) {
        auto shaderName = args.get();
        if (shaderName.empty()) return {"No shader given"};

        std::stringstream ss;

        auto* shaderData = renderContext->shaders.shaderData;

        for (int i = 0; i < Shaders::Count; i++) {
            if (shaderData[i].name == shaderName) {
                reloadShaderProgram(&shaderData[i].program);
                setupShaders(renderContext);
                ss << "Successfully reloaded shader program \"" << shaderName << "\"";
                return {ss.str()};
            }
        }

        ss << "Failed to load shader program \"" << shaderName << "\"";
        
        return {ss.str()};
    }

    Result setFontScale(Args args, RenderContext* ren) {
        auto scaleStr = args.get();
        if (!scaleStr.empty()) {
            double scale_d = strtod(scaleStr.c_str(), NULL);
            if (scale_d > 0.0) {
                ren->debugFont.scale((float)scale_d);
                ren->font.scale((float)scale_d);
                return {"Scaled fonts."};
            }
        }
        return {"Invalid scale."};
    }
}

std::vector<Command> gCommands;

void setCommands(Game* game) {
    using namespace Commands;

    #define REG_COMMAND(name, ...) gCommands.push_back(Command::make(TOSTRING(name), [=](const char* args)->Result{ return name(ArgsList(args), __VA_ARGS__); }))

    auto* ren = game->renderContext;
    auto* state = game->state;
    auto* ecs = &state->ecs;
    auto* textures = &game->renderContext->textures;
    auto* textureArray = &game->renderContext->textureArray;
    auto* shaderManager = &ren->shaders;
    
    REG_COMMAND(kill, state);
    REG_COMMAND(setMode, game);
    REG_COMMAND(setTexture, textures, textureArray);
    REG_COMMAND(showTexture, textures);
    REG_COMMAND(setFontScale, ren);
    REG_COMMAND(setComponent, ecs);
    REG_COMMAND(reloadShader, ren);
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