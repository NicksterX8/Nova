#include "rendering/utils.hpp"

GLuint enableVertexAttribs(ArrayRef<GlVertexDetailedAttribute> attributes) {
    unsigned int stride = 0;
    for (unsigned int i = 0; i < attributes.size(); i++) {
        assert(attributes[i].count > -1);
        stride += attributes[i].count * attributes[i].typeSize;
    }

    size_t offset = 0;
    for (int i = 0; i < attributes.size(); i++) {
        const auto& attribute = attributes[i];
        glVertexAttribPointer(attribute.index, attribute.count, attribute.type, attribute.normalize, stride, (void*)offset);
        glEnableVertexAttribArray(attribute.index);
        offset += attribute.count * attribute.typeSize;
    }
    return stride;
}

GLuint enableVertexAttribsSOA(ArrayRef<GlVertexDetailedAttribute> attributes, unsigned int vertexCount) {
    size_t offset = 0;
    for (int i = 0; i < attributes.size(); i++) {
        const auto& attribute = attributes[i];
        size_t attributeSize = attribute.size();
        auto attributeBufferSize = vertexCount * attributeSize;
        glVertexAttribPointer(attribute.index, attribute.count, attribute.type, attribute.normalize, attributeSize /* stride */, (void*)offset);
        glEnableVertexAttribArray(attribute.index);
        offset += attributeBufferSize;
    }
    return offset /* total size */;
}



GlModel makeModel(
GlBuffer vertexBuffer,
GlBuffer indexBuffer,
const GlVertexFormat& format) {
    GlModel model = GlGenModel();
    model.bindAll();

    enableVertexAttribs(format.attributes);

    GlBufferData(GL_ARRAY_BUFFER, vertexBuffer);
    GlBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);

    return model;
}

GlModelSOA makeModelSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
const GlVertexFormat& format) {
    if (!vertexCount) return {{0, 0, 0}};

    size_t sizePerVertex = format.totalSize();
    size_t totalSize = sizePerVertex * vertexCount;

    GlModel model;
    glGenVertexArrays(1, &model.vao);
    glGenBuffers(1, &model.vbo);
    model.ebo = 0;
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    glBufferData(GL_ARRAY_BUFFER, totalSize, NULL, vertexUsage);
    llvm::SmallVector<size_t> attributeOffsets(format.attributeCount());
    size_t offset = 0;
    for (int i = 0; i < format.attributeCount(); i++) {
        size_t attributeSize = format.attributes[i].size() * vertexCount;
        if (vertexData && vertexData[i]) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, attributeSize, vertexData[i]);
        }
        attributeOffsets[i] = offset;
        offset += attributeSize;
    }

    assert(offset == totalSize);

    enableVertexAttribsSOA(format.attributes, vertexCount);

    return {model, attributeOffsets};
}

GlModel makeModelIndexedSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexFormat& format) {
    if (!vertexCount) return {0, 0, 0};
    auto& attributes = format.attributes;

    // Combine all vertex data arrays into one to buffer into a gl array buffer
    size_t sizePerVertex = 0;
    for (auto attribute : attributes) {
        sizePerVertex += attribute.count * attribute.typeSize;
    }
    size_t totalSize = sizePerVertex * vertexCount;
    char* combinedVertexData = nullptr;
    if (vertexData) {
        combinedVertexData = (char*)malloc(totalSize);
        size_t sum = 0;
        for (int i = 0; i < attributes.size(); i++) {
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

    enableVertexAttribsSOA(format.attributes, vertexCount);

    glBindVertexArray(0);

    return model;
}


GlModel makeModel(
size_t vertexDataSize, const void* vertexData, GLenum vertexUsage,
const GlVertexFormat& format) {
    unsigned int vbo,vao;
    glGenBuffers(1, &vbo);
    glGenVertexArrays(1, &vao);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vertexDataSize != 0)
        glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, vertexUsage);

    enableVertexAttribs(format.attributes);

    glBindVertexArray(0);

    GlModel model;
    model.vbo = vbo;
    model.ebo = 0;
    model.vao = vao;
    return model;
}