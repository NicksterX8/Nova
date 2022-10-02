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


inline void generateQuadVertexIndices(int quadCount, GLushort* indices) {
    for (int i = 0; i < quadCount; i++) {
        GLushort first = i*4;
        int ind = i * 6;
        indices[ind+0] = first+0;
        indices[ind+1] = first+1;
        indices[ind+2] = first+3;
        indices[ind+3] = first+1;
        indices[ind+4] = first+2;
        indices[ind+5] = first+3; 
    }
}

#endif