#ifndef RENDERING_TEXT_INCLUDED
#define RENDERING_TEXT_INCLUDED

#include <glm/vec2.hpp>
#include "../sdl_gl.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.hpp"
#include "utils.hpp"
#include "TexturePacker.hpp"
#include "../My/String.hpp"

enum class HoriAlignment : Uint16 {
    Left=0,
    Center=1,
    Right=2
};
enum class VertAlignment : Uint16 {
    Top=0,
    Middle=1,
    Bottom=2
};

struct TextAlignment {
    HoriAlignment horizontal;
    VertAlignment vertical;

    #define ALIGNMENT_CONST_DECL const static TextAlignment

    ALIGNMENT_CONST_DECL TopLeft;
    ALIGNMENT_CONST_DECL TopCenter;
    ALIGNMENT_CONST_DECL TopRight;
    ALIGNMENT_CONST_DECL MiddleLeft;
    ALIGNMENT_CONST_DECL MiddleCenter;
    ALIGNMENT_CONST_DECL MiddleRight;
    ALIGNMENT_CONST_DECL BottomLeft;
    ALIGNMENT_CONST_DECL BottomCenter;
    ALIGNMENT_CONST_DECL BottomRight;

    #undef ALIGNMENT_CONST_DECL
};

struct TextFormattingSettings {
    glm::vec4 color = {0,0,0,1};
    glm::vec2 scale = glm::vec2(1.0f);
    TextAlignment align = TextAlignment::TopLeft;
    float maxWidth = INFINITY;
    float sizeOffsetScale = 0.0f;
    bool wrapOnWhitespace = false;

    TextFormattingSettings() = default;

    TextFormattingSettings(
        glm::vec4 color, TextAlignment alignment = TextAlignment::TopLeft,
        float scale=1.0f, float maxWidth = INFINITY)
        : color(color), align(alignment), maxWidth(maxWidth) {}

private:
    using Self = TextFormattingSettings;
public:

    static Self Default() {
        return Self();
    }
};

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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

    const static GLint bufferCapacity = 500;
    const static GLint bufferPositionStart = 0;
    const static GLint bufferTexCoordStart = bufferCapacity * 4 * 2 * sizeof(GLfloat);

    glm::vec4 defaultColor;
    glm::vec4 currentColor;
    glm::vec2 *charOffsetBuffer;
    char *charBuffer;
    GLvoid *vertexBuffer;
    Font* font;
    Shader shader;
    GLuint vao,vbo,ebo;
    GLint bufferSize;
    int charBufferSize;
    unsigned int spaceCharAdvance;
    using FormattingSettings = TextFormattingSettings;
    FormattingSettings defaultFormatting;

    struct CharacterRenderData {
        char c;
        glm::ivec2 offset;
    };

    static TextRenderer init(Font* font, Shader shader) {
        TextRenderer self;
        self.bufferSize = 0;
        self.font = font;
        self.defaultColor = {0,0,0,0};
        self.currentColor = {NAN, NAN, NAN, NAN};
        self.shader = shader;
        self.spaceCharAdvance = font->characters[' ' - font->firstChar].advance;

        glGenVertexArrays(1, &self.vao);
        glGenBuffers(1, &self.vbo);
        glGenBuffers(1, &self.ebo);
        glBindVertexArray(self.vao);
        glBindBuffer(GL_ARRAY_BUFFER, self.vbo);
        glBufferData(GL_ARRAY_BUFFER, bufferCapacity * 4 * (2 * sizeof(GLfloat) + 2 * sizeof(GLfloat)), NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferCapacity * 6 * sizeof(GLushort), NULL, GL_STREAM_DRAW);
        GLushort* indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        generateQuadVertexIndices(bufferCapacity, indices);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)(bufferTexCoordStart));
        self.vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // allocate buffers
        self.charBuffer = (char*)malloc(bufferCapacity * sizeof(char));
        self.charOffsetBuffer = (glm::vec2*)malloc(bufferCapacity * sizeof(glm::vec2));
        self.charBufferSize = 0;
        self.defaultFormatting = FormattingSettings::Default();

        return self;
    }

    glm::vec2 outputSize(const char* text, int textLength) {
        float cx = 0.0f,cy = 0.0f;
        float mx = 0.0f,my = 0.0f;
        for (int i = 0; i < textLength; i++) {
            char c = text[i];
            if (c == '\n') {
                cy -= font->lineHeight * (font->face->height >> 6);
                mx = cx > mx ? cx : mx;
                cx = 0.0f;
                continue;
            }
            else if (c == '\t') {
                // tab
                cx += font->tabWidth * (font->characters[' ' - font->firstChar].advance >> 6);
                continue;
            }
            else if (c < font->firstChar || c > font->lastChar) {
                // skip unsupported characters
                continue;
            }

            Font::Character ch = font->characters[c - font->firstChar];

            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            cx += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
        }
        mx = cx > mx ? cx : mx;
        my = cy < my ? cy : my;
        return glm::vec2(mx, -my);
    }

    struct CharacterLayoutData {
        glm::vec2 size;
        glm::vec2 origin;
        int characterCount;
    };

    void alignLine(const char* line, int lineLength, glm::vec2* lineOffsets, HoriAlignment align) {
        if (align != HoriAlignment::Left) {
            // move whole line back to align
            float lineMovement = 0.0f;
            if (align == HoriAlignment::Right) {
                lineMovement = 1;
            } else if (align == HoriAlignment::Center) {
                lineMovement = 1/2.0f;
            }
            for (int j = 0; j < lineLength; j++) {
                lineOffsets[j].x -= lineMovement;
            }
            // reset line
            lineLength = 0;
        }
    }

    CharacterLayoutData formatText(const char* text, int textLength, float maxWidth, char* characters, glm::vec2 *offsets, TextAlignment align = TextAlignment::TopLeft, bool wrapOnSpace=true) {
        float cx = 0.0f,cy = 0.0f;
        float mx = 0.0f,my = 0.0f;
        int charCount = 0;
        int lineLength = 0;
        for (int i = 0; i < textLength; i++) {
            char c = text[i];
            bool whitespaceChar = false; // character that makes whitespace like space, tab, newline
            if (c == ' ') {
                whitespaceChar = true;
                if (cx + (float)(spaceCharAdvance >> 6) >= maxWidth) {
                    goto newline;
                }
                cx += spaceCharAdvance >> 6;
                goto cx_changed;
            }
            else if (c == '\n') {
                whitespaceChar = true;
            newline:
                if (align.horizontal != HoriAlignment::Left) {
                    // shift whole line over to correct alignment location (center or right)
                    float lineMovement = 0.0f;
                    if (align.horizontal == HoriAlignment::Right) {
                        lineMovement = cx;
                    } else if (align.horizontal == HoriAlignment::Center) {
                        lineMovement = cx/2.0f;
                    }
                    for (int j = 0; j < lineLength; j++) {
                        offsets[charCount - j - 1].x -= lineMovement;
                    }
                    // reset line
                    lineLength = 0;
                }

                cy -= font->lineHeight * (font->face->height >> 6);
                mx = cx > mx ? cx : mx;
                cx = 0.0f;
                continue;
            }
            else if (c == '\t') {
                whitespaceChar = true;
                cx += font->tabWidth * (spaceCharAdvance >> 6);
                goto cx_changed;
            }
            else if (c < font->firstChar || c > font->lastChar) {
                // skip unsupported characters
                continue;
            }

            (void)whitespaceChar;

            offsets[charCount] = glm::vec2(cx, cy);
            characters[charCount] = c;
            charCount++;
            lineLength++;

            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            cx += (font->characters[c - font->firstChar].advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
        cx_changed:
            if (cx >= maxWidth) {
                // TODO: this doesn't work
                if (wrapOnSpace) {

                }
                goto newline;
            }
        }

        if (align.horizontal != HoriAlignment::Left) {
            // move whole line back to align
            float lineMovement = 0.0f;
            if (align.horizontal == HoriAlignment::Right) {
                lineMovement = cx;
            } else if (align.horizontal == HoriAlignment::Center) {
                lineMovement = cx/2.0f;
            }
            for (int j = 0; j < lineLength; j++) {
                offsets[charCount - j - 1].x -= lineMovement;
            }
            // reset line
            lineLength = 0;
        }

        glm::vec2 origin = {0,0};

        mx = cx > mx ? cx : mx;
        my = cy < my ? cy : my;

        switch (align.vertical) {
        case VertAlignment::Middle:
            origin.y += my/2.0f;
            break;
        case VertAlignment::Bottom:
            origin.y += my;
            break;
        default:
            break;
        }
        return CharacterLayoutData{
            .size = {mx, -my + (font->face->height >> 6)},
            .origin = origin,
            .characterCount = charCount
        };
    }

    void buffer(int numChars, const char* characters, const glm::vec2* offsets, glm::vec2 pos, glm::vec2 scale, glm::vec4 color) {
        assert(bufferSize + numChars <= bufferCapacity);
        for (int i = 0; i < numChars; i++) {
            char c = characters[i];
            glm::vec2 offset = offsets[i];
            Font::Character ch = font->characters[c - font->firstChar];

            GLfloat x  = pos.x + (offset.x + ch.bearing.x) * scale.x;
            GLfloat y  = pos.y + (offset.y - (ch.size.y - ch.bearing.y)) * scale.y;
            GLfloat x2 = x + ch.size.x * scale.x;
            GLfloat y2 = y + ch.size.y * scale.y;

            GLfloat tx  = (GLfloat)(ch.texPos.x + 0.0f) / font->atlasSize.x;
            GLfloat ty  = (GLfloat)(ch.texPos.y + 0.0f) / font->atlasSize.y;
            GLfloat tx2 = (GLfloat)(ch.texPos.x + ch.size.x + 0.0f) / font->atlasSize.x;
            GLfloat ty2 = (GLfloat)(ch.texPos.y + ch.size.y + 0.0f) / font->atlasSize.y;
            // update VBO for each character
            // rendering glyph texture over quad
            const GLfloat positions[4][2] = {
                {x,  y},
                {x,  y2},
                {x2, y2},
                {x2, y}
            };
            const GLfloat texCoords[4][2] = {
                {tx,  ty2},
                {tx,  ty},
                {tx2, ty},
                {tx2, ty2}
            };
        
            // add to vertex buffer
            memcpy((char*)vertexBuffer + bufferPositionStart + (i + bufferSize) * sizeof(positions), positions, sizeof(positions));
            memcpy((char*)vertexBuffer + bufferTexCoordStart + (i + bufferSize) * sizeof(texCoords), texCoords, sizeof(texCoords));
        }
    }

    inline FRect render(const char* text, glm::vec2 pos) {
        return render(text, pos, defaultFormatting);
    }

    inline FRect render(const char* text, glm::vec2 pos, const TextFormattingSettings& settings) {
        return render(text, strlen(text), pos, settings);
    }

    inline void unionRectInPlace(FRect* r, FRect* b) {
        r->x = MIN(r->x, b->x);
        r->y = MIN(r->y, b->y);
        r->w = MAX(r->w, b->w);
        r->h = MAX(r->h, b->h);
    }

    FRect renderStringBufferAsLines(const My::StringBuffer& strings, glm::vec2 pos, const FormattingSettings& settings) {
        FRect maxRect = {0,0,0,0};
        glm::vec2 offset = {0, 0};
        for (char* str : strings) {
            FRect rect = render(str, pos + offset, settings);
            unionRectInPlace(&maxRect, &rect);
            offset.y -= rect.y;
        }
        return maxRect;
    }

     
    //#define FOR_MY_STRING_BUFFER_REVERSE(name, buffer) char* name = *buffer.end(); name != *buffer.begin(); name = My::StringBuffer::iterator::backward(name)

    //#define FOR_EACH(name, iterable) for (auto name = iterable.)

    FRect renderStringBufferAsLinesReverse(const My::StringBuffer& strings, glm::vec2 pos, const FormattingSettings& settings) {
        FRect maxRect = {0,0,0,0};
        glm::vec2 offset = {0, 0};

        /*
        for (auto str : My::reverse(strings)) {
            FRect rect = render(str, pos + offset, settings);
            unionRectInPlace(&maxRect, &rect);
            offset.y -= rect.y;
        }
        */
        FOR_MY_STRING_BUFFER_REVERSE(str, strings, {
            FRect rect = render(str, pos + offset, settings);
            unionRectInPlace(&maxRect, &rect);
            offset.y -= rect.y;
        });
        return maxRect;
    }

    FRect render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& settings) {
        if (settings.color != currentColor) {
            // can only buffer one color text at a time. may be changed to have color in vertex data.
            if (bufferSize > 0) {
                flush();
            }
            shader.use();
            shader.setVec3("textColor", glm::vec3(settings.color));
            currentColor = settings.color;
        }

        FRect outputRect = {pos.x,pos.y,0,0};
        int textParsed = 0;
        while (textParsed < textLength) {
            if (bufferSize >= bufferCapacity) {
                flush();
            }

            int bufferSpace = bufferCapacity - bufferSize;
            int batchSize = MIN(bufferSpace, textLength);

            auto layoutData = formatText(&text[textParsed], batchSize, settings.maxWidth, charBuffer, charOffsetBuffer, settings.align, settings.wrapOnWhitespace);
            int charsToBuffer = layoutData.characterCount; // not all characters need to actually be buffered (space, tab, etc)
            glm::vec2 origin = pos - layoutData.origin;
            FRect rect = {
                -layoutData.origin.x + pos.x,
                -layoutData.origin.y + pos.y,
                layoutData.size.x,
                layoutData.size.y
            };

            unionRectInPlace(&outputRect, &rect);

            buffer(charsToBuffer, charBuffer, charOffsetBuffer, origin, settings.scale, settings.color);
            textParsed += batchSize;
            bufferSize += charsToBuffer;
        }
        return outputRect;
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
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glDeleteVertexArrays(1, &this->vao);
        glDeleteBuffers(1, &this->vbo);
        glDeleteBuffers(1, &this->ebo);
        free(this->charBuffer);
        free(this->charOffsetBuffer);
    }
};


#endif 