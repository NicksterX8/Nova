#include "TexturePacker.hpp"
#include <vector>

struct Texture {
    std::vector<unsigned char> pixels;
    int width;
    int height;
    int x;
    int y;
};

std::vector<Texture> textures;

void pack(TexturePacker& packer, int width, int height) {
    Texture texture;
    texture.width = width;
    texture.height = height;
    texture.pixels.resize(width * height);
    int i = 0;
    for (auto& pixel : texture.pixels) {
        pixel = 'a' + i;
        i = (i+1) % ('z' - 'a' + 1);
    }
    auto pos = packer.packTexture(texture.pixels.data(), {width, height});
    texture.x = pos.x;
    texture.y = pos.y;
    textures.push_back(texture);
}

int main() {
    TexturePacker packer({12, 3}, 5);
    pack(packer, 40, 39);
    pack(packer, 123, 56);
    pack(packer, 12, 3);
    pack(packer, 1, 54);
    pack(packer, 300, 473);
    unsigned char* buffer = new unsigned char[4096000];
    
    for (auto& texture : textures) {
        auto pos = glm::ivec2{texture.x, texture.y};
        auto size = glm::ivec2{texture.width, texture.height};
        if (packer.extractTexture(pos, size, buffer)) {
            std::cout << "texture size: " << texture.width << ", " << texture.height << ", " << texture.pixels.size() << "\n";
        } else {
            assert(false);
        }
    }
    
}