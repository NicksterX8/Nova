#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <glm/vec2.hpp>
#include "memory.hpp"

#define MY_TEXTURE_ARRAY_WIDTH 128
#define MY_TEXTURE_ARRAY_HEIGHT 128

struct Texture {
    unsigned char* buffer;
    glm::ivec2 size;
    int pixelSize;
};

constexpr Texture NullTexture = {nullptr, {0,0}, 0};

inline Texture createUninitTexture(glm::ivec2 size, int pixelSize) {
    Texture texture;
    texture.size = size;
    texture.buffer = Alloc<unsigned char>(size.x * size.y * pixelSize);
    texture.pixelSize = pixelSize;
    return texture;
}

inline void freeTexture(Texture texture) {
    Free(texture.buffer);
}

Texture loadTexture(const char* filepath, bool flip = true);

void flipSurface(SDL_Surface* surface);

constexpr SDL_PixelFormatEnum interopTexturePixelFormat = SDL_PIXELFORMAT_RGBA32;
constexpr int interopTexturePixelFormatBytes = 4;
constexpr int RGBA32PixelSize = 4;

unsigned int loadGLTexture(const char* filepath);
unsigned int loadGLTexture(Texture rawTexture);

typedef Uint32 TextureID;

namespace TextureIDs {
    enum Extra : TextureID {
        Null=0,
    };
    namespace Tiles {
        enum Tile : TextureID {
            Grass = Null+1,
            Sand,
            Water,
            SpaceFloor,
            Wall,
            lastTile
        };
    }
    enum General : TextureID {
        Inserter = Tiles::lastTile,
        Chest,
        Grenade,
        Player,
        Tree,
        Last
    };

    constexpr TextureID First = Null+1;
    constexpr TextureID NumTextures = Last-1;
}

struct TextureMetaDataStruct {
    const char* filename = NULL;
};

extern TextureMetaDataStruct TextureMetaData[TextureIDs::NumTextures+TextureIDs::First];

struct TextureDataStruct {
    int width;
    int height;
};
extern TextureDataStruct TextureData[TextureIDs::NumTextures+TextureIDs::First];

unsigned int createTextureArray(int width, int height, int depth, SDL_Surface** images);
int setTextureMetadata();
unsigned int makeTextureArray(const char* assetsPath);


#endif