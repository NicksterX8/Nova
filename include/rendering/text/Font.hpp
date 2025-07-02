#pragma once

#include "freetype.hpp"
#include "sdl_gl.hpp"
#include "rendering/Shader.hpp"
#include "rendering/textures.hpp"

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

inline bool isWhitespace(char c) {
    switch (c) {
    case ' ':
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
    static constexpr char MaxChar = 127;

    struct FormattingSettings {
        float tabSpaces = 4.0f; // The width of a tab relative to the size of a space
        float lineSpacing = 1.0f; // space between lines * line height, 1.0 is one font height's distance
    };

    struct AtlasCharacterData {
        static constexpr char ArraySize = MaxChar;
        /* Arrays of character atlas data. Index with char value directly like this: positions['a'] */
        glm::vec<2, uint16_t> positions[ArraySize];
        glm::vec<2, uint16_t> sizes[ArraySize];
        glm::vec<2,  int16_t> bearings[ArraySize];
                    uint16_t   advances[ArraySize];
    };

    FT_Face face = nullptr;
    AtlasCharacterData* characters = nullptr;

    FT_UInt _height = 0;
    GLuint atlasTexture = 0;
    glm::ivec2 atlasSize = {0, 0};

    Shader shader = {};

    FormattingSettings formatting;

    bool usingSDFs = false;
    TextureUnit textureUnit = TextureUnit::Null;

    FT_UInt baseHeight = 0; // the height of the font face before any scaling. The true height is this * scale

    float currentScale = 0.0f;

    Font() = default;

    void load(FT_UInt height, Shader shader, bool useSDFs = false);

    bool loaded() const {
        return face != nullptr;
    }

    void unload();

    FT_UInt linePixelSpacing() const {
        return formatting.lineSpacing * height();
    }

    FT_UInt ascender() const {
        assert(face);
        return face->size->metrics.ascender >> 6;
    }

    // negative value
    FT_Int descender() const {
        assert(face);
        return face->size->metrics.descender >> 6;
    }

    FT_UInt height() const {
        return ascender() - descender();
    }

    glm::ivec2 position(char c) const {
        assert(c >= 0);
        return characters->positions[c];
    }

    glm::ivec2 size(char c) const {
        assert(c >= 0);
        return characters->sizes[c];
    }

    glm::ivec2 bearing(char c) const {
        assert(c >= 0);
        return characters->bearings[c];
    }

    unsigned int advance(char c) const {
        assert(c >= 0 && c <= MaxChar);
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
        load(baseHeight * scale, shader, usingSDFs);
    }

    bool hasKerning() const {
        return FT_HAS_KERNING(face);
    }

    void destroy() {

        unload();
    }
};

Font* newFont(const char* fontfile, FT_UInt baseHeight, bool useSDFs, float scale, Font::FormattingSettings formatting, TextureUnit textureUnit, Shader shader);
