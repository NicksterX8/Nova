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

struct GlVertexDetailedAttribute {
    GLint count;
    GLenum type;
    GLuint typeSize;
    GLubyte index;
    bool normalize;

    constexpr size_t size() const {
        return typeSize * count;
    }
};


struct GlVertexAttribute {
    int count;
    GLenum type;
    GLuint typeSize;
    bool normalize = false;
};

GLuint enableVertexAttribs(ArrayRef<GlVertexDetailedAttribute> attributes);
GLuint enableVertexAttribsSOA(ArrayRef<GlVertexDetailedAttribute> attributes, unsigned int vertexCount);

struct GlVertexFormat {
    llvm::SmallVector<GlVertexDetailedAttribute, 3> attributes;

    constexpr size_t totalSize() const {
        size_t size = 0;
        for (auto attribute : attributes) {
            size += attribute.size();
        }
        return size;
    }

    constexpr size_t attributeCount() const {
        return attributes.size();
    }

    void enable() const {
        enableVertexAttribs(attributes);
    }
};

inline GlVertexFormat GlMakeVertexFormat(GLuint firstAttributeIndex, ArrayRef<GlVertexAttribute> attributes) {
    assert(attributes.size() < UINT8_MAX && "There should never be more than 255 vertex attributes");
    assert(firstAttributeIndex + attributes.size() < UINT8_MAX && "not enough vertex attribute index space");
    GlVertexFormat format;
    format.attributes.reserve(attributes.size());
    for (int i = 0; i < attributes.size(); i++) {
        format.attributes.push_back({.count = attributes[i].count,
            .type = attributes[i].type, .typeSize = attributes[i].typeSize,
            .index = (GLubyte)(i + firstAttributeIndex), .normalize = attributes[i].normalize});
    }
    return format;
}

/*
template<typename T>
GlVertexAttribute GlAttribute(int count, GLenum underlyingGlType, bool normalize = false) {
    return GlVertexAttribute{count, underlyingGlType, sizeof(T), normalize};
}
*/

inline GLuint setupVAO() {
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    return vao;
}

inline GLuint setupVBO(const GlVertexFormat& format, size_t vertexCount, const void* data, GLenum usage) {
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (vertexCount) {
        glBufferData(GL_ARRAY_BUFFER, vertexCount * format.totalSize(), data, usage);
    }
    format.enable();
    return vbo;
}

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


struct GlBuffer {
    size_t size;
    const void* data;
    GLenum usage;
};

inline void GlBufferData(GLenum target, GlBuffer data) {
    glBufferData(target, data.size, data.data, data.usage);
}

GlModel makeModel(GlBuffer vertexBuffer, GlBuffer indexBuffer, const GlVertexFormat& format);

GlModel makeModel(GlBuffer vertexBuffer, const GlVertexFormat& format);

struct GlModelSOA {
    GlModel model;
    llvm::SmallVector<size_t> attributeOffsets;

    void destroy() {
        model.destroy();
        attributeOffsets.clear();
    }
};

GlModelSOA makeModelSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
const GlVertexFormat& vertexFormat);

GlModel makeModelIndexedSOA(
size_t vertexCount, const void * const * vertexData, GLenum vertexUsage,
size_t elementDataSize, const void* elementData, GLenum elementUsage,
const GlVertexFormat& vertexFormat);

template<typename IntType>
inline void generateQuadVertexIndices(int quadCount, IntType* indices) {
    assert(indices);
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
   //if (rect.w == 0 || rect.h == 0) return rect;
    return {rect.x - border.x, rect.y - border.y, rect.w + border.x*2, rect.h + border.y*2};
}

#endif