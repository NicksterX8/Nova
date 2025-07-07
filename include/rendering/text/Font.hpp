#ifndef FONT_INCLUDED
#define FONT_INCLUDED

#include "freetype.hpp"
#include "sdl_gl.hpp"
#include "rendering/Shader.hpp"
#include "rendering/textures.hpp"

namespace Text {

// a texture will be bound on the current active texture unit
inline GLuint loadFontAtlasTexture(Texture atlas, GLint minFilter, GLint magFilter) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        atlas.size.x,
        atlas.size.y,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        atlas.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    return textureID;
}

using Char = char;
using CharIndex = Sint16;

constexpr Char UnsupportedChar = '#';

inline bool isWhitespace(Char c) {
    switch (c) {
    case ' ':
    case '\0':
    case '\t':
    case '\n':
        return true;
    default:
        return false;
    }
}

#define ASCII_FIRST_STANDARD_CHAR ' '
#define ASCII_LAST_STANDARD_CHAR '~'

FT_Face newFontFace(const char* filepath, FT_UInt height);

struct Font {
    static constexpr char MaxChar = ASCII_LAST_STANDARD_CHAR;

    struct FormattingSettings {
        float tabSpaces = 4.0f; // The width of a tab relative to the size of a space
        float lineSpacing = 1.0f; // space between lines * line height, 1.0 is one font height's distance
    };

    struct AtlasCharacterData {
        static constexpr char ArraySize = MaxChar+1;
        /* Arrays of character atlas data. Index with char value directly like this: positions['a'] */
        glm::vec<2, uint16_t> atlasPositions[ArraySize];
        glm::vec<2, uint16_t> sizes[ArraySize];
        glm::vec<2,  int16_t> bearings[ArraySize];
                       float  advances[ArraySize];
    };

    FT_Face face = nullptr;
    AtlasCharacterData* characters = nullptr;

    FT_UInt _height = 0;
    GLuint atlasTexture = 0;
    glm::ivec2 atlasSize = {0, 0};

    FormattingSettings formatting;

    bool usingSDFs = false;
    TextureUnit textureUnit = TextureUnit::Null;

    FT_UInt baseHeight = 0; // the height of the font face before any scaling. The true height is this * scale
    float currentScale = 0.0f;

    int id;

    Font() = default;

    bool load(FT_UInt height, bool useSDFs = false);

    bool loaded() const {
        return face != nullptr;
    }

    void unload();

    FT_UInt linePixelSpacing() const {
        return formatting.lineSpacing * height();
    }

    static bool hasChar(Char c) {
        return (c >= ASCII_FIRST_STANDARD_CHAR && c <= ASCII_LAST_STANDARD_CHAR) || c == '\t' || c == '\n';
    }

    float ascender() const {
        assert(face);
        return face->size->metrics.ascender / 64.0f;
    }

    // negative value
    float descender() const {
        assert(face);
        return face->size->metrics.descender / 64.0f;
    }

    float height() const {
        return ascender() - descender();
    }

    glm::u16vec2 position(char c) const {
        assert(c >= ASCII_FIRST_STANDARD_CHAR && c <= ASCII_LAST_STANDARD_CHAR && "char out of font range!");
        return characters->atlasPositions[c];
    }

    glm::u16vec2 size(char c) const {
        // tab and newline are allowed (they give {0, 0})
        assert(c <= ASCII_LAST_STANDARD_CHAR && "char out of font range!");
        return characters->sizes[c];
    }

    glm::i16vec2 bearing(char c) const {
            assert(c >= ASCII_FIRST_STANDARD_CHAR && c <= ASCII_LAST_STANDARD_CHAR && "char out of font range!");
        return characters->bearings[c];
    }

    float advance(char c) const {
        assert(c >= ASCII_FIRST_STANDARD_CHAR && c <= ASCII_LAST_STANDARD_CHAR && "char out of font range!");
        return characters->advances[c];
    }

    /* Returns a pointer to the set of 4 vectors that make up the character's texture quad on the font atlas
    std::array<TexCoord, 4>* getTexCoords(char c) const {
        return &characterTexCoords[c - firstChar];
    }

    void calculateCharacterTexCoords() {
        glm::vec2 texSize = atlasSize;

        const auto* characterPositions = characters->positions;
        const auto* characterSizes     = characters->sizes;

        for (int i = 0; i < numChars(); i++) {
            char c = firstChar + (char)i;
            auto* coords = characterTexCoords[i].data();
            const glm::vec2 pos  = characterPositions[c];
            const glm::vec2 size = characterSizes[c];

            const float offset = 0.00f;

            auto tx  = (pos.x + offset) / texSize.x;
            auto ty  = (pos.y + offset) / texSize.y;
            auto tx2 = (pos.x + size.x - offset) / texSize.x;
            auto ty2 = (pos.y + size.y - offset) / texSize.y;

            coords[0] = {tx , ty2};
            coords[1] = {tx , ty };
            coords[2] = {tx2, ty };
            coords[3] = {tx2, ty2};
        }
    }*/

    void scale(float scale) {
        if (scale == currentScale) return;
        load(baseHeight * scale, usingSDFs);
    }

    bool hasKerning() const {
        return FT_HAS_KERNING(face);
    }

    void destroy() {

        unload();
    }
};

Font* newFont(const char* fontfile, FT_UInt baseHeight, bool useSDFs, float scale, Font::FormattingSettings formatting, TextureUnit textureUnit);

}

using Text::Font;

#endif