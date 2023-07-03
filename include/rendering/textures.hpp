#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <glm/vec2.hpp>
#include "memory.hpp"
#include "My/HashMap.hpp"
#include "gl.hpp"

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

constexpr SDL_PixelFormatEnum StandardPixelFormat = SDL_PIXELFORMAT_RGBA32;
constexpr int StandardPixelFormatBytes = 4;
constexpr int RGBA32PixelSize = 4;
static_assert(StandardPixelFormatBytes == sizeof(SDL_Color));

unsigned int loadGLTexture(const char* filepath);
unsigned int loadGLTexture(Texture rawTexture);

typedef Uint32 TextureID;

struct TextureMetaData {
    const char* identifier;
    const char* filename;
};

struct TextureData {
    glm::ivec2 size;
};

struct TextureArray {
    glm::ivec2 size; // width and height of each texture
    int depth; // z number of textures
    GLuint texture; // opengl texture id

    void destroy() {
        glDeleteTextures(1, &texture);
    }
};

struct TextureManager {
    My::Vec<TextureMetaData> metadata;
    My::Vec<TextureData> data;

    TextureManager(int numTextures) {
        metadata = My::Vec<TextureMetaData>::Filled(numTextures, {nullptr, nullptr});
        data = My::Vec<TextureData>::Filled(numTextures, {{0,0}});
    }

    void push(TextureID id, const char* stringIdentifier, const char* filename) {
        metadata[id] = TextureMetaData{stringIdentifier, filename};
    }

    void destroy() {
        metadata.destroy();
        data.destroy();
    }
};

unsigned int loadTextureArray(int width, int height, int depth, SDL_Surface** images);

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
        LastPlusOne
    };

    constexpr TextureID First = Null+1;
    constexpr TextureID Last = LastPlusOne - 1;
    constexpr TextureID NumTextures = Last;
    constexpr TextureID NumTextureSlots = Last + 1; // 1 additional slot for null, and possibly others in the future.
    // useful for array sizes and the like
}

// returns -1 on error
inline int getTextureArrayDepth(TextureID id) {
    if (id >= TextureIDs::First && id <= TextureIDs::Last) {
        return (int)id - (int)TextureIDs::First;
    } else {
        return -1;
    }
}

int setTextureMetadata(TextureManager* textureManager);
SDL_Surface** loadTextures(TextureManager* textures, const char* assetsPath);
TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, const char* assetsPath);
int updateTextureArray(TextureArray* textureArray, int depth, SDL_Surface* surface);


#endif