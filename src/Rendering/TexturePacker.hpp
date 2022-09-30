#ifndef TEXTURE_PACKER_INCLUDED
#define TEXTURE_PACKER_INCLUDED

#include <glm/vec2.hpp>
#include <memory>

struct TexturePacker {
    struct TextureNode {
        glm::ivec2 origin; // Top left of the rectangle this node represents
        glm::ivec2 size;   // Size of the rectangle this node represents

        std::unique_ptr<TextureNode> left;  // Left (or top) subdivision
        std::unique_ptr<TextureNode> right; // Right (or bottom) subdivision

        bool empty;    // true if this node is a leaf and is filled

        TextureNode(glm::ivec2 origin, glm::ivec2 size) {
            this->origin = origin;
            this->size = size;
            this->left = nullptr;
            this->right = nullptr;
            this->empty = true;
        }
    };

    unsigned char* buffer;
    glm::ivec2 textureSize;
    std::unique_ptr<TextureNode> rootNode;

private:
    TextureNode* pack(TextureNode* node, glm::ivec2 size);
public:
    TexturePacker(glm::ivec2 startSize) {
        textureSize = startSize;
        buffer = (unsigned char*)malloc(textureSize.x * textureSize.y * sizeof(unsigned char));
        rootNode = std::unique_ptr<TextureNode>(new TextureNode(glm::ivec2(0, 0), glm::ivec2(INT_MAX, INT_MAX)));
    }

    glm::ivec2 packTexture(unsigned char* textureBuffer, glm::ivec2 bufferSize);
    void resizeBuffer(glm::ivec2 newSize);

    void destroy() {
        free(buffer); buffer = nullptr;
    }
};

#endif