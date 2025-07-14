#ifndef RENDERING_RENDERERS_INCLUDED
#define RENDERING_RENDERERS_INCLUDED

#include <glm/vec3.hpp>
#include "gl.hpp"
#include "Shader.hpp"
#include "ADT/ArrayRef.hpp"
#include "rendering/utils.hpp"
#include "textures.hpp"
#include "text/rendering.hpp"

struct ColorVertex {
    glm::vec2 position;
    SDL_Color color;
};

static_assert(sizeof(ColorVertex) == sizeof(GLfloat) * 2 + sizeof(SDL_Color), "not having struct padding is required");

struct QuadRenderer {
    /* Types */
    using TexCoord = glm::vec<2, GLushort>;
    static constexpr TexCoord NullCoord = {-1, -1};
    struct Vertex {
        glm::vec2 pos;
        SDL_Color color;
        TexCoord texCoord;
    };
    static_assert(sizeof(Vertex) == sizeof(glm::vec2) + sizeof(SDL_Color) + 2 * sizeof(GLushort), "no struct padding");
    using Quad = std::array<Vertex, 4>;
    using Height = float;
    using VertexIndexType = GLuint;

    /* Member variables */
    GlModel model;
    struct QuadBatch {
        int bufferIndex;
        int quadCount;
        Height height;
    };
    SmallVector<Quad, 0> quadBuffer;
    SmallVector<QuadBatch, 0> batches;

    float heightIncrementer = 0.0f;
    static constexpr float HeightIncrement = 0.0001f;

    /* Consts */
    static const GLuint maxQuadsPerBatch = 2048;
    static const GLuint maxVerticesPerBatch = maxQuadsPerBatch*4;
    static const GLuint eboIndexCount = maxQuadsPerBatch*6;

    /* Constructors */
    void init();

    /* methods */

    // fastest
    void render(ArrayRef<Quad> quads, Height height);

    void renderVertices(ArrayRef<ColorVertex> vertices, Height height);
    
    Quad* renderManual(int quadCount, Height height);

    void flush(Shader shader, const glm::mat4& transform, TextureUnit texture);

    void destroy() {
        model.destroy();
    }
};

#endif