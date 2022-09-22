#ifndef RENDERING_TEXT_INCLUDED
#define RENDERING_TEXT_INCLUDED

#include <glm/vec2.hpp>
#include "../sdl_gl.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.hpp"
#include "utils.hpp"
#include "texture-packing.hpp"


struct Character {
    GLuint       textureID;  // ID handle of the glyph texture
    glm::ivec2   size;       // Size of glyph
    glm::ivec2   bearing;    // Offset from baseline to left/top of glyph
    unsigned int advance;    // Offset to advance to next glyph
};

FT_Library freetype;

int initFreetype() {
    if (FT_Init_FreeType(&freetype)) {
        LogError("Failed to initialize FreeType library.");
        return -1;
    }

    return 0;
}

void doneFreetype() {
    //FT_Done_FreeType(freetype);
}

struct FontFace {
    FT_Face face;
    Character* characters;
};

struct Font {
    FT_Face face;
    GLuint atlasTexture;
    glm::ivec2 atlasSize;

    struct Character {
        glm::ivec2 texPos;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    } *characters;
};

GLuint generateFontAtlasTexture(const TexturePacker& packer) {
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

void loadFont(Font* font, unsigned char firstChar, unsigned char lastChar) {

    if (lastChar > 127 || firstChar > 127) {
        LogError("Can't load font characters higher than 127!");
        if (lastChar > 127) lastChar = 127;
        if (firstChar > 127) firstChar = 127;
    }

    TexturePacker packer({500, 500});

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    FT_GlyphSlot slot = font->face->glyph;

    FT_Error error;
    
    for (unsigned char c = firstChar; c <= lastChar; c++) {
        // load character glyph 
        if ((error = FT_Load_Char(font->face, c, FT_LOAD_RENDER))) {
            LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
            continue;
        }

        glm::ivec2 texPos = packer.packTexture(slot->bitmap.buffer, glm::ivec2(slot->bitmap.width, slot->bitmap.rows));
  
        // now store character for later use3
        font->characters[c-firstChar] = Font::Character{
            glm::ivec2(0, 0),
            glm::ivec2(slot->bitmap.width, slot->bitmap.rows),
            glm::ivec2(slot->bitmap_left, slot->bitmap_top),
            (unsigned int)slot->advance.x
        };
    }

    font->atlasSize = packer.textureSize;
    font->atlasTexture = generateFontAtlasTexture(packer);
    if (!font->atlasTexture) {
        LogError("Font atlas texture failed to generate!");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

void loadFontFaceCharacters(FT_Face face, Character* firstCharacter, unsigned char firstChar, unsigned char lastChar) {
    if (lastChar > 127 || firstChar > 127) {
        LogError("Can't load font characters higher than 127!");
        if (lastChar > 127) lastChar = 127;
        if (firstChar > 127) firstChar = 127;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    FT_GlyphSlot slot = face->glyph;

    FT_Error error;
    
    for (unsigned char c = firstChar; c <= lastChar; c++) {
        // load character glyph 
        if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER))) {
            LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
            continue;
        }

        // generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            slot->bitmap.width,
            slot->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            slot->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use3
        firstCharacter[c-firstChar] = Character{
            texture, 
            glm::ivec2(slot->bitmap.width, slot->bitmap.rows),
            glm::ivec2(slot->bitmap_left, slot->bitmap_top),
            (unsigned int)slot->advance.x
        };
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

void loadFontFaceCharactersSDF(FT_Face face, Character* firstCharacter, unsigned char firstChar, unsigned char lastChar) {
    if (lastChar > 127 || firstChar > 127) {
        LogError("Can't load font characters higher than 127!");
        if (lastChar > 127) lastChar = 127;
        if (firstChar > 127) firstChar = 127;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    FT_GlyphSlot slot = face->glyph;

    FT_Error error;
    
    for (unsigned char c = firstChar; c <= lastChar; c++) {
        // load character glyph 
        if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER))) {
            LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
            continue;
        }

        if (c != ' ')
            if ((error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF))) {
                LogLocGory(Main, Error, "Failed to load SDF glyph character \'%c\'. Error: %s", c, FT_Error_String(error));
                continue;
            }

        // generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            slot->bitmap.width,
            slot->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            slot->bitmap.buffer
        );
        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // now store character for later use3
        firstCharacter[c-firstChar] = Character{
            texture, 
            glm::ivec2(slot->bitmap.width, slot->bitmap.rows),
            glm::ivec2(slot->bitmap_left, slot->bitmap_top),
            (unsigned int)slot->advance.x
        };
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

void unloadFontFaceCharacters(Character* characters) {
    for (unsigned char c = 0; c < 95; c++) {
        glDeleteTextures(1, &characters[c].textureID); 
    }
}

FontFace loadFontFace(const char* filepath, FT_UInt height, bool useSDFs) {
    FT_Error err = 0;
    FT_Face face = nullptr;
    err = FT_New_Face(freetype, filepath, 0, &face);
    if (err) {
        LogError("Failed to load font face. Filepath: %s. Error: %s", filepath, FT_Error_String(err));
        return {nullptr, nullptr};
    }

    err = FT_Set_Pixel_Sizes(face, 0, height);
    if (err) {
        LogError("Failed to set font face pixel size. FT_Error: %s", FT_Error_String(err));
    }

    FontFace fontFace;
    fontFace.face = face;
    fontFace.characters = new Character[95];

    if (useSDFs) {
        loadFontFaceCharactersSDF(face, fontFace.characters, 32, 127);
    } else {
        loadFontFaceCharacters(face, fontFace.characters, 32, 127);
    }

    return fontFace;
}

void destroyFontFace(FontFace face) {
    unloadFontFaceCharacters(face.characters);
    FT_Done_Face(face.face);
}

vao_vbo_t renderTextInit() {
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return {VAO, VBO};
}

void renderTextGeneric(Shader& shader, FontFace font, unsigned char firstCharacter, unsigned char lastCharacter, const char* text, const float x, const float y, const float scale, glm::vec3 color, float lineSpaceHeight, float tabSpacesWidth) {
    float cx = x;
    float cy = y;

    // activate corresponding render state	
    shader.use();
    shader.setVec3("textColor", color);
    static auto buffers = renderTextInit();
    glBindVertexArray(buffers.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);

    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cy -= lineSpaceHeight * (font.face->height >> 6) * scale;
            cx = x;
            continue;
        }
        else if (*c == '\t') {
            // tab
            cx += tabSpacesWidth * (font.characters[' ' - firstCharacter].advance >> 6) * scale;
            continue;
        }

        if (*c < firstCharacter || *c > lastCharacter) {
            // skip unsupported characters
            continue;
        }

        Character ch = font.characters[*c-firstCharacter];

        if (*c == ' ') {
            cx += (ch.advance >> 6) * scale; 
            continue;
        }

        float xpos = cx + ch.bearing.x * scale;
        float ypos = cy - (ch.size.y - ch.bearing.y) * scale;

        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        cx += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

inline void renderText(Shader& shader, FontFace font, const char* text, glm::vec2 pos, float scale, glm::vec3 color) {
    renderTextGeneric(shader, font, 32, 127, text, pos.x, pos.y, scale, color, 1.9f, 5.0f);
}

void textRenderTest(Shader& shader) {

}

#endif 