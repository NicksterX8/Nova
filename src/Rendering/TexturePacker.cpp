#include "rendering/TexturePacker.hpp"
#include "memory.hpp"
#include "rendering/textures.hpp"

using Node = TexturePackingNode;
constexpr auto NullNode = TexturePackingNullNode;
using Atlas = TexturePackingAtlas;

Atlas makeTexturePackingAtlas(int pixelSize, int reserveCapacity, int reserveSize) {
    Atlas atlas;
    atlas.nodesEmpty.reserve(2 * reserveCapacity);
    atlas.nodes = My::Vec<Node>::WithCapacity(2 * reserveCapacity);
    atlas.atlas = newUninitTexture(glm::ivec2(reserveSize), pixelSize);
    fillTextureBlack(atlas.atlas);

    atlas.nodes.push(Node({0, 0}, {INT_MAX, INT_MAX}));
    atlas.nodesEmpty.push_back(true);
    return atlas;
}

Texture doneTexturePackingAtlas(Atlas* atlas) {
    auto texture = atlas->atlas;
    atlas->nodes.destroy();
    return texture;
}

static int packNode(Atlas* atlas, int nodeIndex, glm::ivec2 size) {
    auto& nodes = atlas->nodes;
    auto& nodesEmpty = atlas->nodesEmpty;
    if (!nodesEmpty[nodeIndex]) {
        // The node is filled, not gonna fit anything else here
        assert(nodes[nodeIndex].left == NullNode && nodes[nodeIndex].right == NullNode);
        return NullNode;
    } else if (nodes[nodeIndex].left != NullNode && nodes[nodeIndex].right != NullNode) {
        // Non-leaf, try inserting to the left and then to the right
        int retval = packNode(atlas, nodes[nodeIndex].left, size);
        if (retval != NullNode) {
            return retval;
        }
        return packNode(atlas, nodes[nodeIndex].right, size);
    } else {
        // This is an unfilled leaf - let's see if we can fill it
        glm::ivec2 realSize = nodes[nodeIndex].size;

        // If we're along a boundary, calculate the actual size
        auto origin = nodes[nodeIndex].origin;
        if (origin.x + nodes[nodeIndex].size.x == INT_MAX) {
            realSize.x = atlas->atlas.size.x - origin.x;
        }
        if (origin.y + nodes[nodeIndex].size.y == INT_MAX) {
            realSize.y = atlas->atlas.size.y - origin.y;
        }

        if (nodes[nodeIndex].size.x == size.x && nodes[nodeIndex].size.y == size.y) {
            // Perfect size - just pack into this node
            nodesEmpty[nodeIndex] = false;
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
                    if (nodes[nodeIndex].size.x > nodes[nodeIndex].size.y) {
                        verticalSplit = false;
                    } else {
                        verticalSplit = true;
                    }
                }

                auto origin = nodes[nodeIndex].origin;
                auto nodeSize = nodes[nodeIndex].size;
                if (verticalSplit) {
                    // Split vertically (left is top)
                    nodes[left] = Node(origin, glm::ivec2(nodeSize.x, size.y));
                    nodes[right] = Node(glm::ivec2(origin.x, origin.y + size.y),
                                            glm::ivec2(nodeSize.x, nodeSize.y - size.y));
                } else {
                    // Split horizontally
                    nodes[left] = Node(origin, glm::ivec2(size.x, nodeSize.y));
                    nodes[right] = Node(glm::ivec2(origin.x + size.x, origin.y), glm::ivec2(nodeSize.x - size.x, nodeSize.y));
                }

                nodes[nodeIndex].left = left;
                nodes[nodeIndex].right = right;
                return packNode(atlas, left, size);
            }
        }
    }
}

glm::ivec2 packTexture(Atlas* atlas, Texture texture, glm::ivec2 padding) {
    if (!texture.buffer || texture.size.x <= 0 || texture.size.y <= 0) {
        return {-1, -1};
    }

    texture.size += padding;

    int nodeIndex = packNode(atlas, Atlas::root, texture.size);
    while (nodeIndex == NullNode) {
        // always keep atlas texture square
        int newSize = MAX(MAX(atlas->atlas.size.x*2, texture.size.x*2), MAX(atlas->atlas.size.y*2, texture.size.y*2));
        atlas->atlas = resizeTexture(atlas->atlas, glm::ivec2(newSize));
        nodeIndex = packNode(atlas, Atlas::root, texture.size);
    }

    Node* node = &atlas->nodes[nodeIndex];

    assert(texture.size.x == node->size.x);
    assert(texture.size.y == node->size.y);

    // Copy the texture to the texture atlas' buffer
    texture.size -= padding;
    copyTexture(atlas->atlas, texture, node->origin);

    return node->origin;
}

Texture packTextures(const int numTextures, const Texture* textures, int pixelSize, glm::ivec2* textureOrigins, glm::ivec2 padding, int startSize) {
    if (!textures || !numTextures) return {nullptr, {0,0}};

    Atlas atlas;
    atlas.nodesEmpty.reserve(2 * numTextures);
    atlas.nodes = My::Vec<Node>::WithCapacity(500);
    atlas.atlas = newUninitTexture(glm::ivec2(startSize), pixelSize);
    fillTextureBlack(atlas.atlas);

    atlas.nodes.push(Node({0, 0}, {INT_MAX, INT_MAX}));
    atlas.nodesEmpty.push_back(true);

    for (int i = 0; i < numTextures; i++) {
        auto origin = packTexture(&atlas, textures[i], padding);
        if (textureOrigins) {
            textureOrigins[i] = origin;
        }
    }

    atlas.nodes.destroy();
    return atlas.atlas;
}