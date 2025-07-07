#ifndef RENDERING_SHADERS_INCLUDED
#define RENDERING_SHADERS_INCLUDED

#include "utils/Log.hpp"
#include "Shader.hpp"

namespace Shaders {
    enum Shader : int {
        Entity = 0,
        Tilemap,
        Point,
        Quad,
        Text,
        SDF,
        Texture,
        Screen,
        Water,
        Count
    };
}

using ShaderID = Shaders::Shader; // Not the same as opengl shader program id
constexpr ShaderID NullShaderID = (ShaderID)-1;

struct ShaderManager {
    Shader shaders[(int)Shaders::Count] = {{}};
    ShaderData shaderData[(int)Shaders::Count] = {{}};

    Shader get(ShaderID shaderID) const {
        if (shaderID < 0 || shaderID >= Shaders::Count) {
            LogError("Tried to get invalid shader id! Id: %d", shaderID);
            return {};
        }
        auto shader = shaders[(int)shaderID];
        if (!shader) {
            LogError("Tried to get bad shader with id %d. Was the shader set up properly?", shaderID);
        }
        return shader;
    }

    // warning: slow - O(n) where n = num of shaders in manager
    // returns: -1 if unable to find
    ShaderID get(std::string shaderName) const {
        for (int i = 0; i < Shaders::Count; i++) {
            if (shaderData[i].name == shaderName) {
                return (ShaderID)i;
            }
        }
        return NullShaderID;
    }

    Shader use(ShaderID shaderID) const {
        auto shader = get(shaderID);
        shader.use();
        return shader;
    }
    
    Shader setup(ShaderID shaderID, const char* name) {
        // if the shader was already set up, dont set it up again
        Shader currentShader = shaders[(int)shaderID];
        if (currentShader) {
            currentShader.use();
            return currentShader;
        }
        ShaderData newShaderData = loadShader(name);
        Shader shader = Shader(newShaderData.program.programID);
        shaderData[(int)shaderID] = newShaderData;
        shaders[(int)shaderID] = shader;
        if (shader) {
            shader.use();
        } else {
            LogError("Failed setting up shader for id %d and name %s", shaderID, name);
        }
        return shader;
    }

    void update(glm::mat4 screenTransform, glm::mat4 worldTransform) const {
        use(Shaders::Text).setMat4("transform", screenTransform);
        use(Shaders::SDF).setMat4("transform", worldTransform);
    }

    const char* getName(ShaderID shaderID) const {
        if (shaderID < 0 || shaderID >= Shaders::Count) {
            LogError("Tried to get invalid shader id! Id: %d", shaderID);
            return nullptr;
        }
        auto& data = shaderData[(int)shaderID];
        return data.name.c_str();
    }

    void destroy() {
        for (auto shader : shaders) {
            shader.destroy();
        }
    }
};

extern const ShaderManager* gShaderManager;

inline Shader useShader(ShaderID shaderID) {
    return gShaderManager->use(shaderID);
}

inline Shader getShader(ShaderID shaderID) {
    return gShaderManager->get(shaderID);
}

inline const char* getShaderName(ShaderID shaderID) {
    return gShaderManager->getName(shaderID);
}

inline ShaderID getShaderIDFromProgramID(GLuint programID) {
    for (int i = 0; i < Shaders::Count; i++) {
        if (gShaderManager->shaders[i].id == programID) {
            return (ShaderID)i;
        }
    }
    return NullShaderID;
}

#endif