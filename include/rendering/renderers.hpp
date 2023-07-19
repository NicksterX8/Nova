#ifndef RENDERING_RENDERERS_INCLUDED
#define RENDERING_RENDERERS_INCLUDED

#include <glm/vec3.hpp>
#include "gl.hpp"
#include "Shader.hpp"
#include "llvm/ArrayRef.h"

struct ColorVertex {
    glm::vec3 position;
    SDL_Color color;
};

static_assert(sizeof(ColorVertex) == sizeof(GLfloat) * 3 + sizeof(SDL_Color), "not having struct padding is required");

struct QuadRenderer {
    using TexCoord = glm::vec<2, GLushort>;
    static constexpr TexCoord NullCoord = {-1, -1};
    struct Vertex {
        glm::vec3 pos;
        SDL_Color color;
        TexCoord texCoord;
    };
    static_assert(sizeof(Vertex) == sizeof(glm::vec3) + sizeof(SDL_Color) + 2 * sizeof(GLushort), "no struct padding");
    using Quad = std::array<Vertex, 4>;

    using VertexIndexType = GLuint;

    /* member variables */
    GlModel model;
    My::Vec<Quad> buffer;

    /* Consts */
    static const GLuint maxQuadsPerBatch = 2048;
    static const GLuint maxVerticesPerBatch = maxQuadsPerBatch*4;
    static const GLuint eboIndexCount = maxQuadsPerBatch*6;

    /* types */
    struct UniformQuad {
        glm::vec3 positions[4];
        SDL_Color color;
    };

    struct UniformQuad2D {
        glm::vec2 positions[4];
        SDL_Color color;
    };

    /* Constructors */
    QuadRenderer() {}

    // TODO: change this to static init() maybe
    QuadRenderer(int nothing) {
        this->buffer = My::Vec<Quad>::WithCapacity(512);

        auto vertexFormat = GlMakeVertexFormat(0, {
            {3, GL_FLOAT, sizeof(GLfloat)}, // pos
            {4, GL_UNSIGNED_BYTE, sizeof(GLubyte), true /* Normalize */}, // color
            {2, GL_UNSIGNED_SHORT, sizeof(GLushort)} // tex coord
        });

        GlBuffer vertexBuffer = {maxQuadsPerBatch * sizeof(Quad), nullptr, GL_STREAM_DRAW};
        GlBuffer indexBuffer = {eboIndexCount * sizeof(VertexIndexType), nullptr, GL_STATIC_DRAW};

        this->model = makeModel(vertexBuffer, indexBuffer, vertexFormat);

        auto* indices = (VertexIndexType*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        generateQuadVertexIndices(maxQuadsPerBatch, indices);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    }

    /* methods */

    // fastest
    void render(ArrayRef<Quad> quads) {
        buffer.push(quads);
    }

    void render(ArrayRef<ColorVertex> vertices) {
        int numQuads = vertices.size() / 4;
        int numVertices = numQuads * 4;
        
        for (int i = 0; i < numQuads; i++) {
            Quad quad;
            for (int p = 0; p < 4; p++) {
                int vertex = i * 4 + p;
                quad[p].pos = vertices[vertex].position;
                quad[p].color = vertices[vertex].color;
                quad[p].texCoord = NullCoord;
            }
            buffer.push(quad);
        }
    }
    
    Quad* renderManual(GLuint quadCount) {
        return buffer.require(quadCount);
    }

    void flush(Shader shader, glm::mat4 transform, TextureUnit texture) {
        if (buffer.empty()) return;

        shader.use();
        shader.setInt("tex", texture);
        shader.setMat4("transform", transform);
        
        glBindVertexArray(model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

        // buffer all the data now
        glBufferData(GL_ARRAY_BUFFER, buffer.size * sizeof(Quad), buffer.data, GL_STREAM_DRAW);
        // batch size is limited by the index buffer size
        int quadsFlushed = 0;
        while (quadsFlushed < buffer.size) {
            int batchSize = MIN(maxQuadsPerBatch, buffer.size);
            glDrawElements(GL_TRIANGLES, 6 * batchSize, GL_UNSIGNED_INT, (void*)(6UL * quadsFlushed));
            quadsFlushed += batchSize;
        }
        buffer.clear();
    }

    void destroy() {
        model.destroy();
        buffer.destroy();
    }
};

#endif