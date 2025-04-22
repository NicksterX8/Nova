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

    void update(glm::mat4 screenTransform, glm::mat4 worldTransform) {
        use(Shaders::Text).setMat4("transform", screenTransform);
    }

    void destroy() {
        for (auto shader : shaders) {
            shader.destroy();
        }
    }
};

#endif