#ifndef RENDERING_UTILS_INCLUDED
#define RENDERING_UTILS_INCLUDED

#include "../sdl.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"

struct vao_vbo_t {
    GLuint vao;
    GLuint vbo;
};

struct vao_vbo_ebo_t {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
};

struct VertexAttribute {
    GLint count;
    GLenum type;
    GLuint typeSize;
};

struct ModelData {
    GLuint VBO;
    GLuint EBO;
    GLuint VAO;

    void destroy() {
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        glDeleteVertexArrays(1, &VAO);
    }
};

unsigned int enableVertexAttribs(const VertexAttribute* attributes, unsigned int count);

ModelData makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const VertexAttribute* attributes, unsigned int numAttributes);

ModelData makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
const VertexAttribute* attributes, unsigned int numAttributes);

#endif