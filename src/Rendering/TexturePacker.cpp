#include "rendering/TexturePacker.hpp"
#include "memory.hpp"

Texture packTextures(const int numTextures, const Texture* textures, VirtualOutput<glm::ivec2> textureOrigins, glm::ivec2 startSize) {
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
                return NullNode;
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
                        nodesEmpty.resize(nodesEmpty.size() + 2, false);

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

    atlas.nodesEmpty.reserve(2 * numTextures);
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
}