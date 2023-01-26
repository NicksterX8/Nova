#include "TexturePacker.hpp"
#include "utils/common-macros.hpp"
/*
TexturePacker::TextureNode* TexturePacker::pack(TextureNode* node, glm::ivec2 size) {
    if (!node->right.empty) {
        // The node is filled, not gonna fit anything else here
        assert(node->left == NullIndex && node->right.index == NullIndex);
        return NULL;
    } else if (node->left != NullIndex && node->right.index != NullIndex) {
        // Non-leaf, try inserting to the left and then to the right
        TextureNode* retval = pack(&nodes[node->left], size);
        if (retval != NULL) {
            return retval;
        }
        return pack(&nodes[node->right.index], size);
    } else {
        // This is an unfilled leaf - let's see if we can fill it
        glm::ivec2 realSize(node->size.x, node->size.y);

        // If we're along a boundary, calculate the actual size
        if (node->origin.x + node->size.x == INT_MAX) {
            realSize.x = atlas.size.x - node->origin.x;
        }
        if (node->origin.y + node->size.y == INT_MAX) {
            realSize.y = atlas.size.y - node->origin.y;
        }

        if (node->size.x == size.x && node->size.y == size.y) {
            // Perfect size - just pack into this node
            node->right.empty = false;
            return node;
        } else {
            glm::vec2 sizeDiff = size - realSize;
            
            if (sizeDiff.x > 0 || sizeDiff.y > 0) {
                // Not big enough
                return nullptr;
            } else {
                // Large enough - split until we get a perfect fit
                int left  = nodes.size;
                int right = nodes.size+1;
                nodes.require(2);

                // Determine how much space we'll have left if we split each way
                int remainX = realSize.x - size.x;
                int remainY = realSize.y - size.y;

                // Split the way that will leave the most room
                bool verticalSplit = remainX < remainY;
                if (remainX == 0 && remainY == 0) {
                    // Edge case - we are are going to hit the border of
                    // the texture atlas perfectly, split at the border instead
                    if (node->size.x > node->size.y) {
                        verticalSplit = false;
                    } else {
                        verticalSplit = true;
                    }
                }

                if (verticalSplit) {
                    // Split vertically (left is top)
                    nodes[left] = TextureNode(node->origin, glm::ivec2(node->size.x, size.y));
                    nodes[right] = TextureNode(glm::ivec2(node->origin.x, node->origin.y + size.y),
                                            glm::ivec2(node->size.x, node->size.y - size.y));
                } else {
                    // Split horizontally
                    nodes[left] = TextureNode(node->origin, glm::ivec2(size.x, node->size.y));
                    nodes[right] = TextureNode(glm::ivec2(node->origin.x + size.x, node->origin.y), glm::ivec2(node->size.x - size.x, node->size.y));
                }

                node->left = left;
                node->right = {right, node->right.empty};
                return pack(&nodes[node->left], size);
            }
        }
    }
}

glm::ivec2 TexturePacker::packTexture(const unsigned char* textureBuffer, glm::ivec2 textureSize) {
    TextureNode* node = pack(root(), textureSize);
    if (node == NULL) {
        do {
            this->resizeBuffer(glm::ivec2(MAX(atlas.size.x*2, textureSize.x*2), MAX(atlas.size.y*2, textureSize.y*2)));
        } while ((node = pack(root(), textureSize)) == nullptr);

        // Note: this assert will be hit if we try to pack a texture larger
        // than the current size of the texture
        assert(node != NULL);
    }

    assert(textureSize.x == node->size.x);
    assert(textureSize.y == node->size.y);

    // Copy the texture to the texture atlas' buffer
    copyTexture(&atlas.buffer[node->origin.y * atlas.size.x + node->origin.x], atlas.size, textureBuffer, textureSize);

    return node->origin;
}

void TexturePacker::resizeBuffer(glm::ivec2 newSize) {
    unsigned char* newBuffer = (unsigned char*)malloc(newSize.y * newSize.x * sizeof(unsigned char));
    copyTexture(newBuffer, newSize, atlas.buffer, atlas.size);
    atlas.size = newSize;

    free(atlas.buffer);
    atlas.buffer = newBuffer;
}
*/

Texture packTextures(const int numTextures, const Texture* textures, glm::ivec2* textureOrigins = nullptr, glm::ivec2 startSize = {-1, -1}) {
    static constexpr int NullNode = -1;

    struct Node {
        glm::ivec2 origin;
        glm::ivec2 size;

        int left;
        int right;

        Node(glm::ivec2 origin, glm::ivec2 size) : origin(origin), size(size), left(NullNode), right(NullNode) {

        }
    };

    struct {
        Texture atlas;
        My::Vec<Node> nodes;
        llvm::BitVector nodesEmpty;

        int pack(int nodeIndex, glm::ivec2 size) {
            Node* node = &nodes[nodeIndex];
            if (!nodesEmpty[nodeIndex]) {
                // The node is filled, not gonna fit anything else here
                assert(node->left == NullNode && node->right == NullNode);
                return NULL;
            } else if (node->left != NullNode && node->right != NullNode) {
                // Non-leaf, try inserting to the left and then to the right
                int retval = pack(node->left, size);
                if (retval != NullNode) {
                    return retval;
                }
                return pack(node->right, size);
            } else {
                // This is an unfilled leaf - let's see if we can fill it
                glm::ivec2 realSize(node->size.x, node->size.y);

                // If we're along a boundary, calculate the actual size
                if (node->origin.x + node->size.x == INT_MAX) {
                    realSize.x = atlas.size.x - node->origin.x;
                }
                if (node->origin.y + node->size.y == INT_MAX) {
                    realSize.y = atlas.size.y - node->origin.y;
                }

                if (node->size.x == size.x && node->size.y == size.y) {
                    // Perfect size - just pack into this node
                    nodesEmpty[node->right] = false;
                    return nodeIndex;
                } else {
                    glm::vec2 sizeDiff = size - realSize;
                    
                    if (sizeDiff.x > 0 || sizeDiff.y > 0) {
                        // Not big enough
                        return NullNode;
                    } else {
                        // Large enough - split until we get a perfect fit
                        int left  = nodes.size;
                        int right = nodes.size+1;
                        nodes.require(2);
                        nodesEmpty.resize(nodesEmpty.size() + 2, true);

                        // Determine how much space we'll have left if we split each way
                        int remainX = realSize.x - size.x;
                        int remainY = realSize.y - size.y;

                        // Split the way that will leave the most room
                        bool verticalSplit = remainX < remainY;
                        if (remainX == 0 && remainY == 0) {
                            // Edge case - we are are going to hit the border of
                            // the texture atlas perfectly, split at the border instead
                            if (node->size.x > node->size.y) {
                                verticalSplit = false;
                            } else {
                                verticalSplit = true;
                            }
                        }

                        if (verticalSplit) {
                            // Split vertically (left is top)
                            nodes[left] = Node(node->origin, glm::ivec2(node->size.x, size.y));
                            nodes[right] = Node(glm::ivec2(node->origin.x, node->origin.y + size.y),
                                                    glm::ivec2(node->size.x, node->size.y - size.y));
                        } else {
                            // Split horizontally
                            nodes[left] = Node(node->origin, glm::ivec2(size.x, node->size.y));
                            nodes[right] = Node(glm::ivec2(node->origin.x + size.x, node->origin.y), glm::ivec2(node->size.x - size.x, node->size.y));
                        }

                        node->left = left;
                        node->right = right;
                        return pack(left, size);
                    }
                }
            }
        }
    } atlas;

    atlas.nodes = My::Vec<Node>::WithCapacity(2 * numTextures);
    atlas.atlas.size = startSize;
    atlas.atlas.buffer = (unsigned char*)malloc(startSize.x * startSize.y * sizeof(unsigned char));

    atlas.nodes.push(Node({0, 0}, {INT_MAX, INT_MAX}));
    int root = 0;

    for (int i = 0; i < numTextures; i++) {
        const auto texture = textures[i];

        int nodeIndex = atlas.pack(root, texture.size);
        while (nodeIndex == NullNode) {
            resizeTexture(atlas.atlas, glm::ivec2(MAX(atlas.atlas.size.x*2, texture.size.x*2), MAX(atlas.atlas.size.y*2, texture.size.y*2)));
            nodeIndex = atlas.pack(root, texture.size);
        }

        Node* node = &atlas.nodes[nodeIndex];

        assert(texture.size.x == node->size.x);
        assert(texture.size.y == node->size.y);

        // Copy the texture to the texture atlas' buffer
        copyTexture(&atlas.atlas.buffer[node->origin.y * atlas.atlas.size.x + node->origin.x], atlas.atlas.size, texture.buffer, texture.size);

        if (textureOrigins)
            textureOrigins[i] = node->origin;
    }

    atlas.nodes.destroy();
    return atlas.atlas;
}