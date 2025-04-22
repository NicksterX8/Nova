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
#include "My/String.hpp"

namespace TextureUnits {
    enum TextureUnit : GLubyte {
        MyTextureArray=0,
        MyTextureAtlas,
        GuiAtlas,
        Font0,
        Font1,
        Random,
        Screen0,
        Screen1,
        Available,
        Null = 255
    };
}

using TextureUnit = TextureUnits::TextureUnit;

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

inline void fillTextureBlack(Texture tex) {
    memset(tex.buffer, 0, tex.size.x * tex.size.y);
}

inline unsigned char* accessTexture(Texture tx, glm::ivec2 pixel) {
    assert(pixel.x >= 0 && pixel.y >= 0);
    assert(pixel.x < tx.size.x && pixel.y < tx.size.y);
    return &tx.buffer[pixel.y * tx.size.x + pixel.x];
}

void copyTexture(Texture dst, Texture src, glm::ivec2 dstOffset = {0, 0});
Texture resizeTexture(Texture texture, glm::ivec2 newSize);

SDL_Surface* loadSurface(const char* filepath, bool flip = true);
void flipSurface(SDL_Surface* surface);

GLuint GlLoadTexture(const char* pixels, glm::ivec2 size, GLint minFilter, GLint magFilter);
GLuint GlLoadSurface(SDL_Surface* surface, GLint minFilter, GLint magFilter);

constexpr SDL_PixelFormatEnum StandardPixelFormat = SDL_PIXELFORMAT_RGBA32;
constexpr int StandardPixelFormatBytes = 4;
constexpr int RGBA32PixelSize = 4;
static_assert(StandardPixelFormatBytes == sizeof(SDL_Color));

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

// a special type of texture
struct Animation {
    TextureID texture;
    IVec2 frameSize;
    int frameCount;
    int updatesPerFrame;
};

struct TextureManager {
    My::Vec<TextureMetaData> metadata;
    My::Vec<TextureData> data;
    My::HashMap<TextureID, Animation> animations;

    TextureManager(int numTextures) {
        metadata = My::Vec<TextureMetaData>::Filled(numTextures, {nullptr, nullptr});
        data = My::Vec<TextureData>::Filled(numTextures, {{0,0}});
        animations = My::HashMap<TextureID, Animation>::WithBuckets(8);
    }

    void add(TextureID id, const char* stringIdentifier, TextureType type, const char* filename, const Animation* animation = nullptr) {
        metadata[id] = TextureMetaData{stringIdentifier, filename, type};
        if (animation) {
            animations.insert(id, *animation);
        }
    }

    void addAnimation(const Animation* animation) {
        animations.insert(animation->texture, *animation);
    }

    TextureID getID(const char* identifier) const {
        for (TextureID id = 0; id < metadata.size; id++) {
            if (My::streq(metadata[id].identifier, identifier)) {
                return id;
            }
        }
        // couldn't find texture
        return 0;
    }

    void destroy() {
        metadata.destroy();
        data.destroy();
        animations.destroy();
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
            GreyFloor,
            GreyFloorOverhang,
            lastTile
        };
    }
    namespace Buildings {
        enum Building : TextureID {
            TransportBelt = Tiles::lastTile,
            TransportBeltAnimation,
            lastBuilding
        };
    }
    namespace GUI {
        enum Gui : TextureID {
            lastGui = Buildings::lastBuilding
        };
    }
    enum General : TextureID {
        Inserter = GUI::lastGui,
        Chest,
        Grenade,
        Player,
        PlayerShadow,
        TreeBottom,
        TreeTop,
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
    My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures> textureDepths;
    glm::ivec2 size; // width and height of each texture
    int depth; // z number of textures
    GLuint texture; // opengl texture id
    TextureUnit unit;

    TextureArray() {}

    TextureArray(glm::ivec2 size, int depth, GLuint texture, TextureUnit unit)
    : size(size), depth(depth), texture(texture), unit(unit) {
        textureDepths = My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures>::WithCapacity(depth);
    }

    void destroy() {
        glDeleteTextures(1, &texture);
        textureDepths.destroy();
    }
};

GLuint GlLoadTextureArray(glm::ivec2 size, ArrayRef<SDL_Surface*> images, GLenum minFilter, GLenum magFilter);
// @typesIncluded can be one or more types (as a bit mask) of textures that should be included in the texture array
TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, TextureType typesIncluded, const char* assetsPath, TextureUnit target);
int updateTextureArray(TextureArray* textureArray, TextureManager* textures, TextureID id, SDL_Surface* surface);

struct TextureAtlas {
    glm::ivec2 size;
    using TexCoord = glm::vec<2, GLushort>;
    static constexpr TexCoord NullCoord = {-1, -1};
    struct Space {
        TexCoord min;
        TexCoord max;

        TexCoord size() const {
            return max - min;
        }

        bool valid() const {
            return !(min == NullCoord || max == NullCoord); // not gonna bother checking every single other one
        }
    };
    My::DenseSparseSet<TextureID, Space, TextureIDs::NumTextures> textureSpaces;
    GLuint texture;
    TextureUnit unit;

    TextureAtlas() {}

    TextureAtlas(glm::ivec2 size, GLuint texture, TextureUnit unit)
    : size(size), texture(texture), unit(unit) {
        textureSpaces = My::DenseSparseSet<TextureID, Space, TextureIDs::NumTextures>::Empty();
    }

    void destroy() {
        glDeleteTextures(1, &texture);
        textureSpaces.destroy();
    }
};

GlSizedTexture GlLoadTextureAtlas(ArrayRef<SDL_Surface*> images, GLint minFilter, GLint magFilter, MutableArrayRef<glm::ivec2> texCoordsOut);
TextureAtlas makeTextureAtlas(TextureManager* textures, TextureType typesIncluded, const char* assetsPath, GLint minFilter, GLint magFilter, TextureUnit target);

// returns -1 on error
inline int getTextureArrayDepth(const TextureArray* textureArray, TextureID id) {
    const int* depth = textureArray->textureDepths.lookup(id);
    return depth ? *depth : -1;
}

inline TextureAtlas::Space getTextureAtlasSpace(const TextureAtlas* textureAtlas, TextureID id) {
    auto* space = textureAtlas->textureSpaces.lookup(id);
    return space ? *space : TextureAtlas::Space{TextureAtlas::NullCoord, TextureAtlas::NullCoord};
}

inline Animation* getAnimation(const TextureManager* textureManager, TextureID id) {
    return textureManager->animations.lookup(id);
}

// first frame is frame 0
inline TextureAtlas::Space getAnimationFrame(const TextureAtlas::Space& atlas, const Animation& animation, int frame) {
    if (frame < 0 || frame >= animation.frameCount) return TextureAtlas::Space{TextureAtlas::NullCoord, TextureAtlas::NullCoord};

    int totalPixelOffset = animation.frameSize.x * frame;
    auto atlasSize = atlas.size();
    int row = totalPixelOffset / atlasSize.x;
    TextureAtlas::TexCoord offset = {totalPixelOffset % atlasSize.x, row * animation.frameSize.y};
    TextureAtlas::TexCoord min = atlas.min + offset;
    
    return {
        min,
        {min.x + animation.frameSize.x, min.y + animation.frameSize.y}
    };
}

inline TextureAtlas::Space getCurrentAnimationFrame(const TextureAtlas::Space& atlas, const Animation& animation) {
    return getAnimationFrame(atlas, animation, (Metadata->getTick() / animation.updatesPerFrame) % animation.frameCount);
}

inline TextureAtlas::Space getAnimationFrameFromAtlas(const TextureAtlas& atlas, const Animation& animation) {
    auto animationSpace = getTextureAtlasSpace(&atlas, animation.texture);
    return getCurrentAnimationFrame(animationSpace, animation);
}

#endif