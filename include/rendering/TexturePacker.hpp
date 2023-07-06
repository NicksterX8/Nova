#ifndef TEXTURE_PACKER_INCLUDED
#define TEXTURE_PACKER_INCLUDED

#include <glm/vec2.hpp>
#include "My/Vec.hpp"
#include "llvm/ArrayRef.h"
#include "llvm/BitVector.h"
#include "rendering/textures.hpp"

// based on this article "https://straypixels.net/texture-packing-for-fonts/" by Edward Lu, modified to be C style

static constexpr int TexturePackingNullNode = -1;

struct TexturePackingNode {
    glm::ivec2 origin;
    glm::ivec2 size;

    int left;
    int right;

    TexturePackingNode(glm::ivec2 origin, glm::ivec2 size)
    : origin(origin), size(size), left(TexturePackingNullNode), right(TexturePackingNullNode) {}
};

// atlas for packing textures
struct TexturePackingAtlas {
    static constexpr int root = 0;

    Texture atlas;
    My::Vec<TexturePackingNode> nodes;
    llvm::BitVector nodesEmpty;
};

TexturePackingAtlas makeTexturePackingAtlas(int pixelSize, int reserveCapacity = 1, int reserveTextureSize = 64);

glm::ivec2 packTexture(TexturePackingAtlas* atlas, Texture texture);

Texture doneTexturePackingAtlas(TexturePackingAtlas* atlas);

/* Texture
 * If the texture is unable to be packed for any reason ,the origin out will be {-1, -1}
 */
Texture packTextures(const int numTextures, const Texture* textures, int pixelSize, glm::ivec2* textureOriginsOut, int reserveTextureSize = 128);

#endif