#ifndef RENDERING_UTILS_INCLUDED
#define RENDERING_UTILS_INCLUDED

#include "../sdl.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils/random.hpp"

struct RenderOptions {
    glm::vec2 size; // in pixels
    float scale;
};

struct vao_vbo_t {
    GLuint vao;
    GLuint vbo;
};

struct GlVertexAttribute {
    GLint count;
    GLenum type;
    GLuint typeSize;
};

struct GlModel {
    GLuint vbo;
    GLuint ebo;
    GLuint vao;

    void bindAll() {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    }

    void destroy() {
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        glDeleteVertexArrays(1, &vao);
    }
};

inline GlModel GlGenModel() {
    GlModel model;
    GLuint buffers[2];

    glGenBuffers(2, buffers);
    glGenVertexArrays(1, &model.vao);
    model.vbo = buffers[0];
    model.ebo = buffers[1];
    return model;
}

unsigned int enableVertexAttribs(const GlVertexAttribute* attributes, unsigned int count);
unsigned int enableVertexAttribsSOA(const GlVertexAttribute* attributes, unsigned int count, unsigned int vertexCount);

GlModel makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes);

GlModel makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes);

GlModel makeModelSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes);

GlModel makeModelIndexedSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes);

template<typename IntType>
inline void generateQuadVertexIndices(int quadCount, IntType* indices) {
    for (int i = 0; i < quadCount; i++) {
        IntType first = i*4;
        int ind = i * 6;
        indices[ind+0] = first+0;
        indices[ind+1] = first+1;
        indices[ind+2] = first+3;
        indices[ind+3] = first+1;
        indices[ind+4] = first+2;
        indices[ind+5] = first+3; 
    }
}

inline glm::vec4 colorConvert(SDL_Color color) {
    return glm::vec4(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
}

inline SDL_Color randomColor() {
    return SDL_Color{(Uint8)randomInt(0, 255), (Uint8)randomInt(0, 255), (Uint8)randomInt(0, 255), (Uint8)randomInt(1, 255)};
}

 inline FRect addBorder(FRect rect, Vec2 border) {
    // dont add borders to empty rectangles
    if (rect.w == 0 || rect.h == 0) return rect;
    return {rect.x - border.x, rect.y - border.y, rect.w + border.x*2, rect.h + border.y*2};
}

#endif