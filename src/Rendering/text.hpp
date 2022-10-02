#ifndef RENDERING_TEXT_INCLUDED
#define RENDERING_TEXT_INCLUDED

#include <glm/vec2.hpp>
#include "../sdl_gl.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.hpp"
#include "utils.hpp"
#include "TexturePacker.hpp"


struct Character {
    GLuint       textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};

extern FT_Library freetype;

inline int initFreetype() {
    if (FT_Init_FreeType(&freetype)) {
        LogError("Failed to initialize FreeType library.");
        return -1;
    }

    return 0;
}

inline void doneFreetype() {
    // seg faults for some reason
    //FT_Done_FreeType(freetype);
}

inline GLuint generateFontAtlasTexture(const TexturePacker& packer) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        packer.textureSize.x,
        packer.textureSize.y,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        packer.buffer
    );
    // set texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

FT_Face newFontFace(const char* filepath, FT_UInt height);

struct Font {
    FT_Face face;
    glm::ivec2 atlasSize;
    GLuint atlasTexture;
    char firstChar;
    char lastChar;

    struct Character {
        glm::ivec2 texPos;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    } *characters;

    void load(const char* fontfile, FT_UInt height, bool useSDFs, char firstChar=32, char lastChar=127);

    void unload();
};

void renderTextGeneric(Shader& shader, const Font& font, const char* text, const float x, const float y, const float scale, glm::vec3 color, float lineSpaceHeight, float tabSpacesWidth);

inline void renderText(Shader& shader, const Font& font, const char* text, glm::vec2 pos, float scale, glm::vec3 color) {
    renderTextGeneric(shader, font, text, pos.x, pos.y, scale, color, 1.9f, 5.0f);
}

#endif 