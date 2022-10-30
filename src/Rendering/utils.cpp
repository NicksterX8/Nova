#include "rendering/utils.hpp"

unsigned int enableVertexAttribs(const VertexAttribute* attributes, unsigned int count) {
    unsigned int stride = 0;
    for (unsigned int i = 0; i < count; i++) {
        stride += attributes[i].count * attributes[i].typeSize;
    }

    size_t offset = 0;
    for (unsigned int i = 0; i < count; i++) {
        glVertexAttribPointer(i, attributes[i].count, attributes[i].type, GL_FALSE, stride, (void*)offset);
        glEnableVertexAttribArray(i);
        offset += attributes[i].count * attributes[i].typeSize;
    }
    return stride;
}

ModelData makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const VertexAttribute* attributes, unsigned int numAttributes) {
    unsigned int vbo,ebo,vao;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, vertexUsage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementDataSize, elementData, elementUsage);

    enableVertexAttribs(attributes, numAttributes);

    glBindVertexArray(0);

    ModelData modelData;
    modelData.VBO = vbo;
    modelData.EBO = ebo;
    modelData.VAO = vao;
    return modelData;
}

ModelData makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
const VertexAttribute* attributes, unsigned int numAttributes) {
    unsigned int vbo,vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vertexDataSize != 0)
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, vertexUsage);

    enableVertexAttribs(attributes, numAttributes);

    glBindVertexArray(0);

    ModelData modelData;
    modelData.VBO = vbo;
    modelData.EBO = 0;
    modelData.VAO = vao;
    return modelData;
}