#include "commands.hpp"
#include "Game.hpp"
#include "rendering/textures.hpp"
#include "utils/FileSystem.hpp"
#include <sstream>

namespace Commands {
    using Result = Command::Result;

    // not very useful tbh
    #define DEF_COMMAND(name, ...) Result name(const char* args, __VA_ARGS__)
    #define DESCRIBE(name, _description) getCommand(TOSTRING(name))->description = _description

    #define RES_ERROR(message) Result{Result::Error, message}
    #define RES_SUCCESS(message) Result{Result::Success, message}

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
                    else if (c == ' ' && (argsLen - i) > 1) {
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

        Result require(int numArgs) {
            if (argc < numArgs) {
                return RES_ERROR("Too few arguments provided!");
            } else if (argc > numArgs) {
                return RES_ERROR("Too many arguments provided!");
            }
            return RES_SUCCESS("");
        }

    };

    using Args = ArgsList;

    #define REQUIRE(num_args) {auto result = args.require(num_args); if (result.type == Result::Error) return result;}

    Result setDebugSetting(Args args, Game* game) {
        auto settingName = args.get();
        auto value = args.get();

        if (value == "1" || value == "true" || value == "y" || value == "True") {
            game->debug->settings[settingName] = 1;
        } else {
            game->debug->settings[settingName] = 0;
        }

        return RES_SUCCESS("Setting changed.");
    }

    Result setMode(Args args, Game* game) {
        auto mode = args.get();

        game->setMode(modeID(mode.c_str()));
        char message[512];
        snprintf(message, 512, "Set mode to %s", mode.c_str());
        return {Result::Success, message};
    }

    Result setTexture(Args args, TextureManager* textures, TextureArray* textureArray) {
        auto textureName        = args.get();
        auto newTextureFilename = args.get();

        char message[512];

        if (textureName.empty()) {
            return RES_ERROR("Failed to set new texture, no texture named.");
        } else if (newTextureFilename.empty()) {
            return RES_ERROR("No new texture given!");
        }

        for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
            TextureMetaData metadata = textures->metadata[id];
            if (My::streq(metadata.identifier, textureName.c_str())) {
                auto filepath = FileSystem.assets.get(newTextureFilename.c_str());
                SDL_Surface* surface = IMG_Load(filepath);
                if (!surface) {
                    snprintf(message, 512, "Couldn't load texture with path %s!", filepath.str);
                    return RES_ERROR(std::string(message));
                }
                int code = updateTextureArray(textureArray, textures, id, surface);
                if (code) {
                    return RES_ERROR("Failed while setting texture!");
                }
                return RES_SUCCESS("Updated texture");
            }
        }
        
        snprintf(message, 512, "Couldn't find texture with name \"%s\"", textureName.c_str());
        return RES_ERROR(std::string(message));
    }

    bool inputIsNumeric(std::string input) {
        bool nonNumeric = false;
        for (auto c : input) {
            if (!std::isdigit(c)) {
                nonNumeric = true;
            }
        }
        return !nonNumeric;
    }

    Entity getEntity(std::string entityIdStr, const EntityWorld* ecs) {
        if (entityIdStr.empty()) return NullEntity;
        EntityID entityID = NullEntity.id;
        // special cases
        if (entityIdStr == "player") {
            return Entities::findNamedEntity(entityIdStr.c_str(), ecs);
        } else {
            if (inputIsNumeric(entityIdStr)) {
                entityID = atoi(entityIdStr.c_str());
                if (entityID > NullEntity.id || entityID < 0) {
                    entityID = NullEntity.id;
                }
            }
        }
        EntityVersion version = NullEntity.version;
        if (entityID != NullEntity.id) {
            version = ecs->GetEntityVersion(entityID);
        }
        if (version == NullEntity.version) {
            // no entity is currently in existence with that id
            entityID = NullEntity.id;
        }
        return Entity(entityID, version);
    }

    Result setComponent(Args args, GameState* state) {
        REQUIRE(3);
        auto entityIdStr = args.get();
        auto componentName = args.get();
        auto componentValueStr = args.get();

        auto* ecs = &state->ecs;

        Entity entity = getEntity(entityIdStr, &state->ecs);
        if (!ecs->EntityExists(entity)) {
            return RES_ERROR("That entity doesn't exist!");
        }

        ComponentID componentID = ecs->GetComponentIdFromName(componentName.c_str());
        if (componentID == NullComponentID) {
            return RES_ERROR("Invalid component!");
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
        if (componentSize <= 0) return RES_ERROR("Invalid component!");

        bool success;
        // position is annoying. maybe change one day
        if (componentID == ECS::getID<EC::Position>()) {
            Vec2 oldPos = ((EC::Position*)component)->vec2();
            success = ecs->GetComponentValueFromStr(componentID, componentValueStr, component);
            entityPositionChanged(state, entity, oldPos);
        } else {
            success = ecs->GetComponentValueFromStr(componentID, componentValueStr, component);
        }
        if (success) {
            return RES_SUCCESS("Successfully set component value");
        }
        return RES_ERROR("Failed to parse component value");
    }

    Result kill(Args args, GameState* state) {
        auto target = args.get();
        bool isName = !inputIsNumeric(target);
        if (isName) {
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
                return false;
            });
            char message[512];
            snprintf(message, 512, "Killed %d %ss", numDestroyed, target.c_str());
            return RES_SUCCESS(std::string(message));
        } else {
            // entity id
            Entity entity = getEntity(target, &state->ecs);
            if (entity.NotNull()) {
                state->ecs.Destroy(entity);
                return RES_SUCCESS("Killed!!");
            }
            return RES_ERROR("Entity not found!");
        }
    }

    Result showTexture(Args args, const TextureManager* textures) {
        if (g.debugTexture) {
            glDeleteTextures(1, &g.debugTexture);
        }

        auto texture = args.get();

        TextureID texID = textures->getID(texture.c_str());
        auto& metadata = textures->metadata[texID];

        g.debugTexture = GlLoadSurface(loadSurface(FileSystem.assets.get(metadata.filename)), GL_NEAREST, GL_NEAREST);
        return RES_SUCCESS("Showing texture");
    }

    Result reloadShader(Args args, RenderContext* renderContext) {
        auto shaderName = args.get();
        if (shaderName.empty()) return RES_ERROR("No shader given");

        std::stringstream ss;

        auto shaderID = renderContext->shaders.get(shaderName);
        if (shaderID == -1) {
            ss << "Failed to find shader program with name \"" << shaderName << ".\"";
            return RES_ERROR(ss.str());
        }
        auto* program = &renderContext->shaders.shaderData[shaderID].program;
        int err = reloadShaderProgram(program);
        if (err) {
            ss << "Failed to reload shader.";
            return RES_ERROR(ss.str());
        }
        renderContext->shaders.shaders[shaderID] = Shader(program->programID);
        setConstantShaderUniforms(*renderContext);
        ss << "Successfully reloaded shader program \"" << shaderName << ".\"";
        return RES_SUCCESS(ss.str());
    }

    Result setFontScale(Args args, RenderContext* ren) {
        auto scaleStr = args.get();
        if (!scaleStr.empty()) {
            double scale_d = strtod(scaleStr.c_str(), NULL);
            if (scale_d > 0.0) {
                scaleAllFonts(ren->fonts, (float)scale_d);
                return RES_SUCCESS("Scaled fonts.");
            }
        }
        return RES_ERROR("Invalid scale.");
    }

    Result clone(Args args, EntityWorld* ecs) {
        REQUIRE(1);
        auto entityIdStr = args.get();
        Entity entity = getEntity(entityIdStr, ecs);
        if (entity.Null()) return RES_ERROR("No entity found");
        Entity clone  = Entities::clone(ecs, entity);
        if (ecs->EntityExists(clone)) {
            return RES_SUCCESS(string_format("Entity successfully cloned! ID: %d", clone.id));
        }
        return RES_ERROR("Failed to clone entity!");
    }

    Result setCameraFocus(Args args, CameraFocus* cameraFocus, const EntityWorld* ecs) {
        REQUIRE(2);
        auto targetType = args.get();
        if (targetType == "entity") {
            auto entityId = args.get();
            Entity entity = getEntity(entityId, ecs);
            if (entity.Null()) {
                return RES_ERROR(string_format("No entity found for id '%s'!", entityId.c_str()));
            }
            if (!ecs->EntityHas<EC::Position>(entity)) {
                return RES_ERROR("The entity cannot be focused on, as it has no position.");
            }
            cameraFocus->focus = CameraEntityFocus{entity, ecs->EntityHas<EC::Dynamic>(entity)};
            return RES_SUCCESS("Focus set.");
        } else if (targetType == "point") {
            // TODO:
            LogWarn("Unfinished");
        }
        return RES_ERROR("Invalid target to focus on.");
    }

    Result getTick(Args args, int nothing) {
        REQUIRE(0);
        (void)nothing;

        return RES_SUCCESS(string_format("%llu", Metadata->getTick()));
    } 

    Result commands(Args args, int) {
        std::string output = "";
        
        for (auto& command : gCommands) {
            output += command.name;
            if (!command.description.empty()) {
                output += " - description: " + command.description;
            }
            output += "\n";
        }

        return RES_SUCCESS(output);
    }

    Result clear(Args args, GUI::Console* console) {
        console->log.clear();
        return RES_SUCCESS("");
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
    DESCRIBE(kill, "Kills an entity with the given name");
    REG_COMMAND(setMode, game);
    DESCRIBE(setMode, "Set the current mode of the engine. Options include blah blah blah");
    REG_COMMAND(setTexture, textures, textureArray);
    DESCRIBE(setTexture, "Set a new texture with a given texture name.\nArgument 1: Texture name\n Argument 2: New texture file");
    REG_COMMAND(showTexture, textures);
    REG_COMMAND(setFontScale, ren);
    REG_COMMAND(setComponent, state);
    REG_COMMAND(clone, ecs);
    REG_COMMAND(reloadShader, ren);
    REG_COMMAND(setCameraFocus, &game->cameraFocus, ecs);
    REG_COMMAND(getTick, 0);
    REG_COMMAND(commands, 0);
    REG_COMMAND(clear, &game->gui->console);
    REG_COMMAND(setDebugSetting, game);
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