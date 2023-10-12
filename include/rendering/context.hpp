#ifndef RENDERING_CONTEXT_INCLUDED
#define RENDERING_CONTEXT_INCLUDED

#include "../sdl_gl.hpp"
#include "Shader.hpp"
#include "shaders.hpp"
#include "text.hpp"
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

struct RenderSystem;

struct RenderContext {
    SDL_Window* window = NULL;
    SDL_GLContext glCtx = NULL;

    TextureAtlas textureAtlas;
    TextureArray textureArray;
    TextureManager textures{0};
    ShaderManager shaders;

    Font font;
    Font debugFont;
    QuadRenderer guiQuadRenderer;
    TextRenderer guiTextRenderer;
    GuiRenderer guiRenderer;
    QuadRenderer worldQuadRenderer;
    TextRenderer worldTextRenderer;
    GuiRenderer worldGuiRenderer;

    RenderSystem* renderSystem;

    ChunkModel chunkModel;
    ChunkBuffer chunkBuffer;

    RenderBuffer framebuffer;
    GlModelSOA screenModel;

    RenderContext(SDL_Window* window, SDL_GLContext glContext)
    : window(window), glCtx(glContext) {}
};

#endif