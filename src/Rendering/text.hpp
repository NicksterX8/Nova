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
    float tabWidth;
    float lineHeight;

    struct Character {
        glm::ivec2 texPos;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    } *characters;

    Font() = default;

    Font(float tabWidth, float lineHeight) : tabWidth(tabWidth), lineHeight(lineHeight) {

    }

    void load(const char* fontfile, FT_UInt height, bool useSDFs, char firstChar=32, char lastChar=127);

    void unload();
};

struct TextRenderer {
    const static GLint bufferCharacterCapacity = 500;
    const static GLint bufferPositionStart = 0;
    const static GLint bufferTexCoordStart = bufferCharacterCapacity * 4 * 2 * sizeof(GLfloat);

    glm::vec4 defaultColor;
    glm::vec4 currentColor;
    GLvoid* vertexBuffer;
    Font* font;
    Shader shader;
    GLuint vao,vbo,ebo;
    GLint bufferSize;

    static TextRenderer init(Font* font, Shader shader) {
        TextRenderer self;
        self.bufferSize = 0;
        self.font = font;
        self.defaultColor = {0,0,0,0};
        self.currentColor = {NAN, NAN, NAN, NAN};
        self.shader = shader;

        glGenVertexArrays(1, &self.vao);
        glGenBuffers(1, &self.vbo);
        glGenBuffers(1, &self.ebo);
        glBindVertexArray(self.vao);
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferCharacterCapacity * 4 * (2 * sizeof(GLfloat) + 2 * sizeof(GLushort)), NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferCharacterCapacity * 6 * sizeof(GLushort), NULL, GL_STREAM_DRAW);
        GLushort* indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        generateQuadVertexIndices(bufferCharacterCapacity, indices);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_UNSIGNED_SHORT, GL_TRUE, 2 * sizeof(GLushort), (void*)(bufferTexCoordStart));
        self.vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return self;
    }

    int buffer(const char* text, int textLength, const glm::vec2 pos, const float scale, glm::vec4 color) {
        int charactersBuffered = 0;
        float cx = pos.x;
        float cy = pos.y;
        while (charactersBuffered < textLength) {
            if (bufferSize >= bufferCharacterCapacity) {
                flush();
            }
            int bufferSpace = bufferCharacterCapacity - bufferSize;
            int batchSize = textLength < bufferSpace ? textLength : bufferSpace;

            //if (color != currentColor) {
                shader.use();
                shader.setVec3("textColor", glm::vec3(color));
                currentColor = color;
            //}

            for (int i = 0; i < batchSize; i++) {
                char c = text[i];
                if (c == '\n') {
                    cy -= font->lineHeight * (font->face->height >> 6) * scale;
                    cx = pos.x;
                    continue;
                }
                else if (c == '\t') {
                    // tab
                    cx += font->tabWidth * (font->characters[' ' - font->firstChar].advance >> 6) * scale;
                    continue;
                }
                else if (c < font->firstChar || c > font->lastChar) {
                    // skip unsupported characters
                    continue;
                }

                Font::Character ch = font->characters[c-font->firstChar];
                if (c == ' ') {
                    cx += (ch.advance >> 6) * scale; 
                    continue;
                }

                GLfloat x  = cx + ch.bearing.x * scale;
                GLfloat y  = cy - (ch.size.y - ch.bearing.y) * scale;
                GLfloat x2 = x + ch.size.x * scale;
                GLfloat y2 = y + ch.size.y * scale;

                GLushort tx  = (GLushort)ch.texPos.x;
                GLushort ty  = (GLushort)ch.texPos.y;
                GLushort tx2 = (GLushort)(ch.texPos.x + ch.size.x);
                GLushort ty2 = (GLushort)(ch.texPos.y + ch.size.y);
                // update VBO for each character
                // rendering glyph texture over quad
                GLfloat positions[4][2] = {
                    {x,  y},
                    {x,  y2},
                    {x2, y2},
                    {x2, y}
                };
                GLushort texCoords[4][2] = {
                    {tx,  ty2},
                    {tx,  ty},
                    {tx2, ty},
                    {tx2, ty2}
                };
            
                // add to vertex buffer
                memcpy((char*)vertexBuffer + bufferPositionStart + (i + bufferSize) * sizeof(positions), positions, sizeof(positions));
                memcpy((char*)vertexBuffer + bufferTexCoordStart + (i + bufferSize) * sizeof(texCoords), texCoords, sizeof(texCoords));
                
                // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
                cx += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
            }

            bufferSize += batchSize;
            charactersBuffered += batchSize;
        }
    }

    void flush() {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        // render quads
        shader.use();
        glBindTexture(GL_TEXTURE_2D, font->atlasTexture);
        glDrawElements(GL_TRIANGLES, 6 * bufferSize, GL_UNSIGNED_SHORT, NULL);
        vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        bufferSize = 0;
    }

    void destroy() {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }
};

void renderTextGeneric(Shader& shader, const Font& font, const char* text, const float x, const float y, const float scale, glm::vec3 color, float lineSpaceHeight, float tabSpacesWidth);

inline void renderText(TextRenderer& textRenderer, const char* text, glm::vec2 pos, float scale, glm::vec3 color) {
    //renderTextGeneric(shader, font, text, pos.x, pos.y, scale, color, 1.9f, 5.0f);
    textRenderer.buffer(text, strlen(text), pos, scale, glm::vec4(color, 1.0f));
}


#endif 