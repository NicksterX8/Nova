#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <glm/vec2.hpp>
#include "memory.hpp"
#include "My/HashMap.hpp"
#include "My/SparseSets.hpp"
#include "gl.hpp"

struct Texture {
    unsigned char* buffer;
    glm::ivec2 size;
    int pixelSize;
};

constexpr Texture NullTexture = {nullptr, {0,0}, 0};

inline Texture newUninitTexture(glm::ivec2 size, int pixelSize) {
    Texture texture;
    texture.size = size;
    texture.buffer = Alloc<unsigned char>(size.x * size.y * pixelSize);
    texture.pixelSize = pixelSize;
    return texture;
}

inline Texture newTexture(glm::ivec2 size, int pixelSize, const unsigned char* buffer) {
    Texture texture;
    texture.size = size;
    texture.buffer = Alloc<unsigned char>(size.x * size.y * pixelSize);
    memcpy(texture.buffer, buffer, size.x * size.y * pixelSize);
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

typedef Uint16 TextureID;
typedef Uint32 TextureType; // also usable as bit mask

struct TextureMetaData {
    const char* identifier;
    const char* filename;
    TextureType type;
};

struct TextureData {
    glm::ivec2 size;
};

struct TextureManager {
    My::Vec<TextureMetaData> metadata;
    My::Vec<TextureData> data;

    TextureManager(int numTextures) {
        metadata = My::Vec<TextureMetaData>::Filled(numTextures, {nullptr, nullptr});
        data = My::Vec<TextureData>::Filled(numTextures, {{0,0}});
    }

    void add(TextureID id, const char* stringIdentifier, TextureType type, const char* filename) {
        metadata[id] = TextureMetaData{stringIdentifier, filename, type};
    }

    TextureID getID(const char* identifier) {
        // TODO
        return 0;
    }

    void destroy() {
        metadata.destroy();
        data.destroy();
    }
};

struct GlSizedTexture {
    GLuint texture;
    glm::ivec2 size;
};

SDL_Surface* loadTexture(TextureID id, TextureManager* textures, const char* basePath);
int setTextureMetadata(TextureManager* textureManager);

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
    namespace GUI {
        enum Gui : TextureID {
            lastGui = Tiles::lastTile
        };
    }
    enum General : TextureID {
        Inserter = GUI::lastGui,
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

namespace TextureTypes {
    enum TextureTypes : TextureType {
        World = 1,
        Gui = 2,
        Other = 4
    };
}

struct TextureArray {
    glm::ivec2 size; // width and height of each texture
    int depth; // z number of textures
    GLuint texture; // opengl texture id
    My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures> textureDepths;

    TextureArray() {}

    TextureArray(glm::ivec2 size, int depth, GLuint texture) : size(size), depth(depth), texture(texture) {
        textureDepths = My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures>::WithCapacity(depth);
    }

    void destroy() {
        glDeleteTextures(1, &texture);
        textureDepths.destroy();
    }
};

GLuint loadTextureArray(glm::ivec2 size, ArrayRef<SDL_Surface*> images, ArrayRef<TextureID> ids, My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures>& textureDepths);
// @typesIncluded can be one or more types (as a bit mask) of textures that should be included in the texture array
TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, TextureType typesIncluded, const char* assetsPath);
int updateTextureArray(TextureArray* textureArray, TextureManager* textures, TextureID id, SDL_Surface* surface);

struct TextureAtlas {
    glm::ivec2 size;
    GLuint texture;
    using TexCoord = glm::vec<2, GLushort>;
    static constexpr TexCoord NullCoord = {UINT16_MAX, UINT16_MAX};
    struct Space {
        TexCoord min;
        TexCoord max;

        bool valid() const {
            return !(min == NullCoord || max == NullCoord); // not gonna bother checking every single other one
        }
    };
    My::DenseSparseSet<TextureID, Space, TextureIDs::NumTextures> textureSpaces;

    TextureAtlas() {}

    TextureAtlas(glm::ivec2 size, GLuint texture) : size(size), texture(texture) {
        textureSpaces = My::DenseSparseSet<TextureID, Space, TextureIDs::NumTextures>::Empty();
    }

    void destroy() {
        glDeleteTextures(1, &texture);
        textureSpaces.destroy();
    }
};

GlSizedTexture loadTextureAtlas(ArrayRef<SDL_Surface*> images, MutableArrayRef<glm::ivec2> texCoordsOut);
TextureAtlas makeTextureAtlas(TextureManager* textures, TextureType typesIncluded, const char* assetsPath);

// returns -1 on error
inline int getTextureArrayDepth(const TextureArray* textureArray, TextureID id) {
    const int* depth = textureArray->textureDepths.lookup(id);
    return depth ? *depth : -1;
}

inline TextureAtlas::Space getTextureAtlasSpace(const TextureAtlas* textureAtlas, TextureID id) {
    auto* space = textureAtlas->textureSpaces.lookup(id);
    return space ? *space : TextureAtlas::Space{TextureAtlas::NullCoord, TextureAtlas::NullCoord};
}

#endif