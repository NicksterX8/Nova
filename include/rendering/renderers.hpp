#ifndef RENDERING_RENDERERS_INCLUDED
#define RENDERING_RENDERERS_INCLUDED

#include <glm/vec3.hpp>
#include "gl.hpp"
#include "Shader.hpp"

struct ColorVertex {
    glm::vec3 position;
    SDL_Color color;
};

static_assert(sizeof(ColorVertex) == sizeof(GLfloat) * 3 + sizeof(SDL_Color), "not having struct padding is required");

struct ColorVertex2D {
    glm::vec2 position;
    SDL_Color color;
};

struct ColorQuadRenderBuffer {
    using Vertex = ColorVertex;
    using Vertex2D = ColorVertex2D;

    /* member variables */
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    Vertex* vertexBuffer;
    GLuint storedQuads = 0;
    Shader shader = {};
    float z = 0.0f;

    /* Consts */
    static const GLuint maxQuadsPerBatch = 10000;
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
    ColorQuadRenderBuffer() {}

    ColorQuadRenderBuffer(Shader shader) {
        this->shader = shader;

        storedQuads = 0;

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, maxVerticesPerBatch * sizeof(Vertex), NULL, GL_STREAM_DRAW);
        vertexBuffer = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboIndexCount * sizeof(GLuint), NULL, GL_STATIC_DRAW);
        GLuint* indices = static_cast<GLuint*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

        for (GLuint i = 0; i < maxQuadsPerBatch; i++) {
            GLuint first = i*4;
            GLuint ind = i * 6;
            indices[ind+0] = first+0;
            indices[ind+1] = first+1;
            indices[ind+2] = first+3;
            indices[ind+3] = first+1;
            indices[ind+4] = first+2;
            indices[ind+5] = first+3; 
        }

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        const size_t stride = 3 * sizeof(GLfloat) + sizeof(SDL_Color);
        static_assert(sizeof(SDL_Color) == 4 * sizeof(GLubyte), "sdl color bits match opengl");

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        // normalize sdl_color (u32) to vec4
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
    }

    /* methods */
    inline Vertex* getBufferTop() {
        return &vertexBuffer[storedQuads * 4];
    }

    inline GLuint needBufferSpace(GLuint count) {
        if (count > maxQuadsPerBatch) {
            LogWarn("Needed %u quads rendered, more than buffer capacity", count);
        }
        if (storedQuads + count > maxQuadsPerBatch) {
            flush();
            return (count < maxQuadsPerBatch) ? count : maxQuadsPerBatch;
        }
        return count;
    }

    // fastest
    GLuint buffer(GLuint quadCount, const Vertex* quads) {
        quadCount = needBufferSpace(quadCount);

        memcpy(getBufferTop(), quads, quadCount * 4 * sizeof(Vertex));

        storedQuads += quadCount;
        return quadCount;
    }

    GLuint buffer(GLuint quadCount, const Vertex2D* quads) {
        quadCount = needBufferSpace(quadCount);

        Vertex* top = getBufferTop();
        for (GLuint i = 0; i < quadCount*4; i++) {
            top[i] = {glm::vec3(quads[i].position, z), quads[i].color};
        }

        storedQuads += quadCount;
        return quadCount;
    }

    // slower to buffer
    GLuint bufferSOA(GLuint quadCount, const glm::vec3* vertexPositions, const glm::vec4* vertexColors) {
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

    
    Vertex* bufferManual(GLuint quadCount) {
        GLuint available = needBufferSpace(quadCount);
        if (available < quadCount) {
            return NULL;
        }

        Vertex* top = getBufferTop();
        storedQuads += quadCount;
        return top;
    }

    void flush() {
        if (storedQuads == 0) return;

        shader.use();

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        GL::logErrors();
        glUnmapBuffer(GL_ARRAY_BUFFER);
        GL::logErrors();
        glDrawElements(GL_TRIANGLES, 6 * storedQuads, GL_UNSIGNED_INT, 0);
        GL::logErrors();

        storedQuads = 0;
        vertexBuffer = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        GL::logErrors();
    }

    void destroy() {
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }
};

using QuadRenderer = ColorQuadRenderBuffer;

#endif