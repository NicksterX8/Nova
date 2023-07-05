#include "rendering/utils.hpp"

unsigned int enableVertexAttribs(const GlVertexAttribute* attributes, unsigned int count) {
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

unsigned int enableVertexAttribsSOA(const GlVertexAttribute* attributes, unsigned int count, unsigned int vertexCount) {
    size_t offset = 0;
    for (unsigned int i = 0; i < count; i++) {
        size_t attributeSize = attributes[i].count * attributes[i].typeSize;
        auto attributeBufferSize = vertexCount * attributeSize;
        glVertexAttribPointer(i, attributes[i].count, attributes[i].type, GL_FALSE, attributeSize /* stride */, (void*)offset);
        glEnableVertexAttribArray(i);
        offset += attributeBufferSize;
    }
    return offset /* total size */;
}

GlModel makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes) {
    GLuint vbo,ebo,vao;
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glGenVertexArrays(1, &vao);
    GL::logErrors();
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, vertexUsage);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementDataSize, elementData, elementUsage);

    enableVertexAttribs(attributes, numAttributes);

    glBindVertexArray(0);

    GlModel modelData;
    modelData.vbo = vbo;
    modelData.ebo = ebo;
    modelData.vao = vao;
    return modelData;
}

struct Buffer {
    void* data;
    size_t size;
};

GlModel makeModelSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes) {
    if (!vertexCount) return {0, 0, 0};

    // Combine all vertex data arrays into one to buffer into a gl array buffer
    size_t sizePerVertex = 0;
    for (int i = 0; i < numAttributes; i++) {
        sizePerVertex += attributes[i].count * attributes[i].typeSize;
    }
    size_t totalSize = sizePerVertex * vertexCount;
    char* combinedVertexData = nullptr;
    if (vertexData) {
        combinedVertexData = (char*)malloc(totalSize);
        size_t sum = 0;
        for (int i = 0; i < numAttributes; i++) {
            size_t attributeSize = attributes[i].count * attributes[i].typeSize * vertexCount;
            memcpy(combinedVertexData + sum, vertexData[i], attributeSize);
            sum += attributeSize;
        }
        assert(totalSize == sum);
    }

    GlModel model;
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &model.vbo);
    model.ebo = 0;
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, totalSize, combinedVertexData, vertexUsage);
    free(combinedVertexData);

    enableVertexAttribsSOA(attributes, numAttributes, vertexCount);

    glBindVertexArray(0);

    return model;
}

GlModel makeModelIndexedSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes) {
    if (!vertexCount) return {0, 0, 0};

    // Combine all vertex data arrays into one to buffer into a gl array buffer
    size_t sizePerVertex = 0;
    for (int i = 0; i < numAttributes; i++) {
        sizePerVertex += attributes[i].count * attributes[i].typeSize;
    }
    size_t totalSize = sizePerVertex * vertexCount;
    char* combinedVertexData = nullptr;
    if (vertexData) {
        combinedVertexData = (char*)malloc(totalSize);
        size_t sum = 0;
        for (int i = 0; i < numAttributes; i++) {
            size_t attributeSize = attributes[i].count * attributes[i].typeSize * vertexCount;
            memcpy(combinedVertexData + sum, vertexData[i], attributeSize);
            sum += attributeSize;
        }
        assert(totalSize == sum);
    }

    GlModel model = GlGenModel();
    model.bindAll();
    glBufferData(GL_ARRAY_BUFFER, totalSize, combinedVertexData, vertexUsage);
    free(combinedVertexData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementDataSize, elementData, elementUsage);

    enableVertexAttribsSOA(attributes, numAttributes, vertexCount);

    glBindVertexArray(0);

    return model;
}


GlModel makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
const GlVertexAttribute* attributes, unsigned int numAttributes) {
    unsigned int vbo,vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vertexDataSize != 0)
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, vertexUsage);

    enableVertexAttribs(attributes, numAttributes);

    glBindVertexArray(0);

    GlModel model;
    model.vbo = vbo;
    model.ebo = 0;
    model.vao = vao;
    return model;
}