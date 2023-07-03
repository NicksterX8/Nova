#ifndef RENDERING_CONTEXT_INCLUDED
#define RENDERING_CONTEXT_INCLUDED

#include "../sdl_gl.hpp"
#include "Shader.hpp"
#include "text.hpp"
#include "My/Vec.hpp"
#include "renderers.hpp"
#include "rendering/gui.hpp"

namespace TextureUnits {
    enum E : GLuint {
        MyTextureArray=0,
        Text=1,
        Single
    };
}

using TextureUnit = TextureUnits::E;

namespace Shaders {
    enum Shader : int {
        Entity = 0,
        Tilemap,
        Point,
        Color,
        Text,
        SDF,
        Texture,
        Count
    };
}

using ShaderID = Shaders::Shader; // Not the same as opengl shader program id

struct ShaderManager {
    Shader shaders[(int)Shaders::Count] = {{}};

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
    
    Shader setup(ShaderID shaderID, const char* filename) {
        Shader shader = loadShader(filename);
        shaders[(int)shaderID] = shader;
        if (shader) {
            shader.use();
        } else {
            LogError("Failed setting up shader for id %d and filename %s", shaderID, filename);
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

using ChunkVertexMap = My::HashMap<IVec2, int32_t, IVec2Hash>;

struct ChunkBuffer {
    ChunkVertexMap map = ChunkVertexMap::Empty();
    size_t chunksStored = 0;
    static constexpr size_t bufferChunkCapacity = (8 * 1024 * 1024) / (CHUNKSIZE * CHUNKSIZE * 4 * 5 * sizeof(GLfloat));
};

struct RenderContext {
    SDL_Window* window = NULL;
    SDL_GLContext glCtx = NULL;
    TextureManager textures{0};
    TextureArray textureArray;
    ShaderManager shaders;

    Font font;
    Font debugFont;
    TextRenderer textRenderer;
    QuadRenderer quadRenderer;
    GuiRenderer guiRenderer;

    GlModel chunkModel;
    ChunkBuffer chunkBuffer;

    RenderContext(SDL_Window* window, SDL_GLContext glContext)
    : window(window), glCtx(glContext) {}
};

#endif