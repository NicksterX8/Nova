#ifndef RENDERING_CONTEXT_INCLUDED
#define RENDERING_CONTEXT_INCLUDED

#include <unordered_map>
#include "../sdl_gl.hpp"
#include "Shader.hpp"
#include "shaders.hpp"
#include "text/rendering.hpp"
#include "My/HashMap.hpp"
#include "My/Vec.hpp"
#include "renderers.hpp"
#include "rendering/gui.hpp"

using ChunkVertexMap = My::HashMap<IVec2, int32_t, IVec2Hash>;

struct ChunkBuffer {
    ChunkVertexMap map = ChunkVertexMap::Empty();
    size_t chunksStored = 0;
    static constexpr size_t bufferChunkCapacity = (4 * 1024 * 1024) / (CHUNKSIZE * CHUNKSIZE * 4 * 5 * sizeof(GLfloat));
};

struct ChunkModel {
    using Pos = glm::vec2;
    using TexCoord = GLushort[4];

    GLuint vao;
    GLuint positionVbo;
    GLuint texCoordVbo;

    void destroy() {
        glDeleteVertexArrays(1, &vao);
        GLuint buffers[2] = {positionVbo, texCoordVbo};
        glDeleteBuffers(2, buffers);
    }
};

struct RenderBuffer {
    GLuint fbo;
    GLuint rbo;
    GLuint colorTexture;
    GLuint velocityTexture;
};

struct FontManager {
    std::unordered_map<std::string, Font*> fonts;
    float fontScale = 1.0f;

    void add(const char* name, Font* font) {
        fonts.insert({std::string(name), font});
    }

    Font* get(const char* fontName) {
        auto it = fonts.find(fontName);
        if (it != fonts.end()) return it->second;
        else { 
            LogError("No font found with name '%s'", fontName);
            return nullptr;
        }
    }

    const Font* get(const char* fontName) const {
        auto it = fonts.find(fontName);
        if (it != fonts.end()) return it->second;
        else {
            LogError("Font not found with name \"%s\".", fontName);
            return nullptr;
        }
    }

    void destroy() {
        for (auto font : fonts) {
            if (font.second) { 
                font.second->destroy();
                delete font.second;
            }
        }
        // idk if i need to delete fonts map
    }
};

extern const FontManager* Fonts;

namespace ECS {
    namespace System {
        struct SystemManager;
    }
}

struct RenderContext {
    SDL_Window* window = nullptr;
    SDL_GLContext glCtx = nullptr;

    TextureAtlas textureAtlas;
    TextureArray textureArray;
    TextureManager textures{0};
    ShaderManager shaders;
    FontManager fonts;
    
    QuadRenderer guiQuadRenderer;
    TextRenderer guiTextRenderer;
    GuiRenderer guiRenderer;
    QuadRenderer worldQuadRenderer;
    TextRenderer worldTextRenderer;
    GuiRenderer worldGuiRenderer;

    ECS::System::SystemManager* ecsRenderSystems;

    ChunkModel chunkModel;
    ChunkBuffer chunkBuffer;

    RenderBuffer framebuffer;
    GlModelSOA screenModel;

    RenderContext(SDL_Window* window, SDL_GLContext glContext)
    : window(window), glCtx(glContext) {}
};

void scaleAllFonts(FontManager&, float scale);

#endif