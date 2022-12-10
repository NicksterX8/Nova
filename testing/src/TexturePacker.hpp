#ifndef TEXTURE_PACKER_INCLUDED
#define TEXTURE_PACKER_INCLUDED

#include <glm/vec2.hpp>
#include <memory>
#include "My/Vec.hpp"
#include "llvm/ArrayRef.h"
#include "llvm/BitVector.h"

#include <iostream>

// based on this article "https://straypixels.net/texture-packing-for-fonts/" by Edward Lu, with some minor modifications
// TODO: Move away from using std::unique_ptr. Use a better method of allocation. Add a destroy method instead of using destructor

/*
struct TextureBuffer {
    unsigned char* buffer;
    glm::ivec2 bufferSize;
    TextureNode* rootNode;
};
*/

inline void copyTexture(unsigned char* dst, glm::ivec2 dstSize, const unsigned char* src, glm::ivec2 srcSize, glm::ivec2 srcOrigin = {0, 0}) {
    assert(srcOrigin.x < srcSize.x && srcOrigin.y < srcSize.y && "Can't make a copy texture bigger than the source texture");
    for (int row = 0; row < dstSize.y; row++) {
        int srcRow = row + srcOrigin.y;
        memcpy(&dst[row * dstSize.x], &src[srcRow * srcSize.x + srcOrigin.x], dstSize.x);
    }
}

struct Texture {
    unsigned char* buffer;
    glm::ivec2 size;
};

inline Texture resizeTexture(Texture texture, glm::ivec2 newSize) {
    unsigned char* newBuffer = (unsigned char*)malloc(newSize.y * newSize.x * sizeof(unsigned char));
    if (!newBuffer) return texture;
    copyTexture(newBuffer, newSize, texture.buffer, texture.size);
    free(texture.buffer);
    return Texture{newBuffer, newSize};
}

Texture packTextures(const int numTextures, const Texture* textures, glm::ivec2* textureOrigins = nullptr, glm::ivec2 startSize = {-1, -1});

/*
struct TextureNode {
    glm::ivec2 origin; // Top left of the rectangle this node represents
    glm::ivec2 size;   // Size of the rectangle this node represents

    int left;  // Left (or top) subdivision
    struct {
        int index:31;
        int empty:1; // true if this node is a leaf and is filled
    } right; // Right (or bottom) subdivision

    static constexpr int NullIndex = -1;

    TextureNode(glm::ivec2 origin, glm::ivec2 size) {
        this->origin = origin;
        this->size = size;
        this->left = -1;
        this->right.index = -1;
        this->right.empty = true;
    }
};

class TexturePacker {

    *
    unsigned char* buffer;
    glm::ivec2 bufferSize;
    *
    Texture atlas;
    int rootNode;
    My::Vec<TextureNode> nodes;

    TextureNode* pack(TextureNode* node, glm::ivec2 size);

    TextureNode* root() {
        return &this->nodes[rootNode];
    }
public:
    unsigned char* getBuffer() const {
        return this->atlas.buffer;
    }

    glm::ivec2 getBufferSize() const {
        return this->atlas.size;
    }

    int newNode(glm::ivec2 origin, glm::ivec2 size) {
        nodes.push(TextureNode(origin, size));
        return nodes.size-1;
    }

    TexturePacker(glm::ivec2 startSize, int startNodeCapacity = 0) {
        atlas.size = startSize;
        atlas.buffer = (unsigned char*)malloc(atlas.size.x * atlas.size.y * sizeof(unsigned char));
        nodes = My::Vec<TextureNode>::WithCapacity(startNodeCapacity);
        rootNode = newNode({0, 0}, {INT_MAX, INT_MAX});
    }

    glm::ivec2 packTexture(const unsigned char* textureBuffer, glm::ivec2 bufferSize);
    void resizeBuffer(glm::ivec2 newSize);

    bool extractTexture(glm::ivec2 pos, glm::ivec2 size, unsigned char* outBuffer) const {
        if (pos.x + size.x > atlas.size.x || pos.y + size.y > atlas.size.y ||
            pos.x < 0 || size.x < 0 || pos.y < 0 || size.y < 0
        ) return false;
        
        // copy rows one by one
        copyTexture(outBuffer, size, this->atlas.buffer, this->atlas.size, pos);
        return true;
    }

    void destroy() {
        std::cout << "Texture packer destructor called!\n";
        free(atlas.buffer);
        nodes.destroy();
    }
};
*/

#endif