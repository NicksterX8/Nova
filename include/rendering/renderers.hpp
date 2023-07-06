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
    struct Vertex {
        glm::vec3 pos;
        SDL_Color color;
        TexCoord texCoord;
    };
    static_assert(sizeof(Vertex) == sizeof(glm::vec3) + sizeof(SDL_Color) + 2 * sizeof(GLushort), "no struct padding");
    using Quad = std::array<Vertex, 4>;

    /* member variables */
    GlModel model;
    My::Vec<Quad> buffer;

    /* Consts */
    static const GLuint maxQuadsPerBatch = 1000;
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

    QuadRenderer(int nothing) {
        this->buffer = My::Vec<Quad>::WithCapacity(500);

        constexpr GlVertexAttribute attributes[] = {
            {3, GL_FLOAT, sizeof(GLfloat)},
            {4, GL_UNSIGNED_BYTE, sizeof(GLubyte)},
            {2, GL_UNSIGNED_SHORT, sizeof(GLushort)}
        };

        this->model = makeModel(maxQuadsPerBatch * sizeof(Quad), nullptr, GL_STREAM_DRAW,
            maxQuadsPerBatch * 6 * sizeof(GLuint), nullptr, GL_STATIC_DRAW,
            attributes, 3);
        model.bindAll(); 

        glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboIndexCount * sizeof(GLuint), NULL, GL_STATIC_DRAW);
        GLuint* indices =(GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

        generateQuadVertexIndices(maxQuadsPerBatch, indices);

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        const size_t stride = 3 * sizeof(GLfloat) + sizeof(SDL_Color) + 2 * sizeof(GLushort);
        static_assert(sizeof(SDL_Color) == 4 * sizeof(GLubyte), "sdl color bits match opengl");

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        // normalize sdl_color (u32) to vec4
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // texture coord
        glVertexAttribPointer(2, 2, GL_UNSIGNED_SHORT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat) + 4 * sizeof(GLubyte)));
        glEnableVertexAttribArray(2);
    }

    /* methods */

    // fastest
    void render(ArrayRef<Quad> quads) {
        buffer.push(quads);
    }

    /*
    void render(GLuint quadCount, const Vertex2D* quads) {
        buffer.reserve(buffer.size + quadCount);

        Vertex* top = getBufferTop();
        for (GLuint i = 0; i < quadCount*4; i++) {
            buffer.push({glm::vec3(quads[i].position, z), quads[i].color});
        }

        storedQuads += quadCount;
        return quadCount;
    }

    // slower to buffer
    void renderSOA(GLuint quadCount, const glm::vec3* vertexPositions, const glm::vec4* vertexColors) {
        quadCount = needBufferSpace(quadCount);

        Vertex* top = &vertexBuffer[storedQuads * 4];
        for (GLuint i = 0; i < quadCount*4; i++) {
            memcpy(&top[i].position, &vertexPositions[i], 3 * sizeof(GLfloat));
        }
        for (GLuint i = 0; i < quadCount*4; i++) {
            memcpy(&top[i].color, &vertexColors[i], 4 * sizeof(GLfloat));
        }

        storedQuads += quadCount;
        return quadCount;
    }

    GLuint bufferUniform(GLuint quadCount, const UniformQuad* quads) {
        quadCount = needBufferSpace(quadCount);

        Vertex* top = getBufferTop();
        for (GLuint i = 0; i < quadCount*4; i++) {
            top[i].position = quads[i/4].positions[i%4];
            top[i].color = quads[i/4].color;
        }

        storedQuads += quadCount;
        return quadCount;
    }

    GLuint bufferUniform(GLuint quadCount, const UniformQuad2D* quads) {
        quadCount = needBufferSpace(quadCount); // renders none if 

        Vertex* top = getBufferTop();
        for (GLuint i = 0; i < quadCount*4; i++) {
            top[i].position = glm::vec3(quads[i/4].positions[i%4], z);
            top[i].color = quads[i/4].color;
        }

        storedQuads += quadCount;
        return quadCount;
    }
    */
    
    Quad* renderManual(GLuint quadCount) {
        return buffer.require(quadCount);
    }

    void flush(Shader shader, GLuint texture) {
        if (buffer.size == 0) return;

        shader.use();

        glBindVertexArray(model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ebo);
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