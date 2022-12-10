#ifndef RENDERING_TEXT_INCLUDED
#define RENDERING_TEXT_INCLUDED

#include <glm/vec2.hpp>
#include "../sdl_gl.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.hpp"
#include "utils.hpp"
#include "TexturePacker.hpp"
#include "My/String.hpp"

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
    TextAlignment align = TextAlignment::TopLeft;
    float maxWidth = INFINITY;
    float sizeOffsetScale = 0.0f;
    bool wrapOnWhitespace = false;

    TextFormattingSettings() = default;

    TextFormattingSettings(
        TextAlignment alignment,
        float maxWidth = INFINITY)
        : align(alignment), maxWidth(maxWidth) {}

private:
    using Self = TextFormattingSettings;
public:

    static Self Default() {
        return Self();
    }
};

struct TextRenderingSettings {
    glm::vec4 color = {0,0,0,1};
    glm::vec2 scale = glm::vec2(1.0f);

    TextRenderingSettings() = default;

    TextRenderingSettings(glm::vec4 color, glm::vec2 scale = glm::vec2{1.0f})
    : color(color), scale(scale) {}
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

inline void quitFreetype() {
    // seg faults for some reason
    FT_Done_FreeType(freetype);
}

inline GLuint loadFontAtlasTexture(Texture atlas) {
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return textureID;
}

FT_Face newFontFace(const char* filepath, FT_UInt height);

struct Font {
    FT_Face face;
    glm::ivec2 atlasSize;
    float tabWidth; // times space char advance
    float lineHeight; // times the font face height
    GLuint atlasTexture;
    unsigned short spaceCharAdvance; // advance in pixels

    /*
    struct Character {
        glm::ivec2 texPos;
        glm::ivec2 size;
        glm::ivec2 bearing;
        unsigned int advance;
    } *characters;
    */

    struct AtlasCharacterData {
        glm::ivec2* positions;
        glm::ivec2* sizes;
        glm::ivec2* bearings;
        unsigned short* advances;
    };

    AtlasCharacterData characters;

    char firstChar;
    char lastChar;

    Font() = default;

    Font(float tabWidth, float lineHeight) : tabWidth(tabWidth), lineHeight(lineHeight) {

    }

    void load(const char* fontfile, FT_UInt height, bool useSDFs, char firstChar=32, char lastChar=127);

    void unload();

    glm::ivec2 position(char c) const {
        assert(c >= firstChar && c <= lastChar && "character not available in font!");
        return characters.positions[c - firstChar];
    }

    glm::ivec2 size(char c) const {
        assert(c >= firstChar && c <= lastChar && "character not available in font!");
        return characters.sizes[c - firstChar];
    }

    glm::ivec2 bearing(char c) const {
        assert(c >= firstChar && c <= lastChar && "character not available in font!");
        return characters.bearings[c - firstChar];
    }

    unsigned int advance(char c) const {
        assert(c >= firstChar && c <= lastChar && "character not available in font!");
        return characters.advances[c - firstChar];
    }
};

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;

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
    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;

    std::array<glm::vec2, 4>* characterTexCoords;

    struct CharacterRenderData {
        char c;
        glm::ivec2 offset;
    };

    void calculateCharacterTexCoords() {
        const float textureWidth = font->atlasSize.x;
        const float textureHeight = font->atlasSize.y;

        for (char c = font->firstChar; c <= font->lastChar; c++) {
            glm::vec2* coords = characterTexCoords[c].data();
            const auto pos = font->position(c);
            const auto size = font->size(c);

            auto tx = (GLfloat)(pos.x + 0.0f) / textureWidth;
            auto ty = (GLfloat)(pos.y + 0.0f) / textureHeight;
            auto tx2 = (GLfloat)(pos.x + size.x + 0.0f) / textureWidth;
            auto ty2 = (GLfloat)(pos.y + size.y + 0.0f) / textureHeight;

            coords[0] = {tx,  ty2};
            coords[1] = {tx,  ty};
            coords[2] = {tx2, ty};
            coords[3] = {tx2, ty2};
        }
    }

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

        self.calculateCharacterTexCoords();

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
                cx += font->tabWidth * (font->spaceCharAdvance >> 6);
                continue;
            }
            else if (c < font->firstChar || c > font->lastChar) {
                // skip unsupported characters
                continue;
            }

            // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
            cx += (font->advance(c) >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
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

    CharacterLayoutData formatText(const char* text, int textLength, char* characters, glm::vec2 *offsets, FormattingSettings settings) {
        float cx = 0.0f,cy = 0.0f;
        float mx = 0.0f,my = 0.0f;
        int charCount = 0;
        int lineLength = 0;
        for (int i = 0; i < textLength; i++) {
            char c = text[i];
            bool whitespaceChar = false; // character that makes whitespace like space, tab, newline
            if (c == ' ') {
                whitespaceChar = true;
                if (cx + (float)(font->spaceCharAdvance >> 6) >= settings.maxWidth) {
                    goto newline;
                }
                cx += font->spaceCharAdvance >> 6;
                goto cx_changed;
            }
            else if (c == '\n') {
                whitespaceChar = true;
            newline:
                if (settings.align.horizontal != HoriAlignment::Left) {
                    // shift whole line over to correct alignment location (center or right)
                    float lineMovement = 0.0f;
                    if (settings.align.horizontal == HoriAlignment::Right) {
                        lineMovement = cx;
                    } else if (settings.align.horizontal == HoriAlignment::Center) {
                        lineMovement = cx/2.0f;
                    }
                    for (int j = 0; j < lineLength; j++) {
                        offsets[charCount - j - 1].x -= lineMovement;
                    }
                    // reset line
                    lineLength = 0;
                }

                cy -= font->lineHeight * (font->face->height >> 6);
                mx = MAX(MAX(mx, cx), settings.maxWidth);
                cx = 0.0f;
                continue;
            }
            else if (c == '\t') {
                whitespaceChar = true;
                cx += font->tabWidth * (font->spaceCharAdvance >> 6);
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
            cx += (font->advance(c) >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
        cx_changed:
            if (cx >= settings.maxWidth) {
                // TODO: this doesn't work
                /*
                if (settings.wrapOnSpace) {

                }*/
                goto newline;
            }
        }

        if (settings.align.horizontal != HoriAlignment::Left) {
            // move whole line back to align
            float lineMovement = 0.0f;
            if (settings.align.horizontal == HoriAlignment::Right) {
                lineMovement = cx;
            } else if (settings.align.horizontal == HoriAlignment::Center) {
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

        switch (settings.align.vertical) {
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

    void buffer(int numChars, const char* chars, const glm::vec2* offsets, glm::vec2 pos, RenderingSettings settings) {
        assert(bufferSize + numChars <= bufferCapacity);
        for (int i = 0; i < numChars; i++) {
            char c = chars[i];
            glm::vec2 offset = offsets[i];
            auto bearing = font->bearing(c);
            auto size = font->size(c);

            GLfloat x  = pos.x + (offset.x + bearing.x) * settings.scale.x;
            GLfloat y  = pos.y + (offset.y - (size.y - bearing.y)) * settings.scale.y;
            GLfloat x2 = x + size.x * settings.scale.x;
            GLfloat y2 = y + size.y * settings.scale.y;

            auto coords = characterTexCoords[c - font->firstChar];
            // update VBO for each character
            // rendering glyph texture over quad
            const GLfloat positions[4][2] = {
                {x,  y},
                {x,  y2},
                {x2, y2},
                {x2, y}
            };  
        
            // add to vertex buffer
            memcpy((char*)vertexBuffer + bufferPositionStart + (i + bufferSize) * sizeof(positions), positions, sizeof(positions));
            const float* texCoords = (float*)characterTexCoords[c - font->firstChar].data();
            memcpy((char*)vertexBuffer + bufferTexCoordStart + (i + bufferSize) * 4 * sizeof(glm::vec2), texCoords, 4 * sizeof(glm::vec2));
        }
    }

    inline FRect render(const char* text, glm::vec2 pos) {
        return render(text, pos, defaultFormatting, defaultRendering);
    }

    inline FRect render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings) {
        return render(text, strlen(text), pos, formatSettings, renderSettings);
    }

    inline void unionRectInPlace(FRect* r, FRect* b) {
        r->x = MIN(r->x, b->x);
        r->y = MIN(r->y, b->y);
        r->w = MAX(r->x + r->w, b->x + b->w) - r->x;
        r->h = MAX(r->y + r->h, b->y + b->h) - r->y;
    }

    FRect renderStringBufferAsLinesReverse(const My::StringBuffer& strings, glm::vec2 pos, My::Vec<glm::vec4> colors, glm::vec2 scale, FormattingSettings formatSettings) {
        FRect maxRect = {pos.x,pos.y,0,0};
        glm::vec2 offset = {0, 0};
        int i = 0;
        strings.forEachStrReverse([&](char* str){
            FRect rect = render(str, pos + offset, formatSettings, TextRenderingSettings{colors[colors.size - 1 - i], scale});
            this->flush();
            unionRectInPlace(&maxRect, &rect);
            offset.y += rect.h;
            // do line break
            offset.y += font->lineHeight * (font->face->height >> 6);
            i++;
        });
        return maxRect;
    }

    FRect render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings) {
        if (currentColor != renderSettings.color || true) {
            // can only buffer one color text at a time. may be changed to have color in vertex data.
            if (bufferSize > 0) {
                flush();
            }
            shader.use();
            shader.setVec3("textColor", glm::vec3(renderSettings.color));
            currentColor = renderSettings.color;
        }

        Vec2 bigMin = {pos.x, pos.y};
        Vec2 maxSize = {0, 0};
        int textParsed = 0;
        while (textParsed < textLength) {
            if (bufferSize >= bufferCapacity) {
                flush();
            }

            int bufferSpace = bufferCapacity - bufferSize;
            int batchSize = MIN(bufferSpace, textLength);

            auto layoutData = formatText(&text[textParsed], batchSize, charBuffer, charOffsetBuffer, formatSettings);
            int charsToBuffer = layoutData.characterCount; // not all characters need to actually be buffered (space, tab, etc)
            Vec2 origin = pos - layoutData.origin;
            Vec2 size = layoutData.size;
            //bigMin.x = MIN(bigMin.x, origin.x);
            //bigMin.y = MIN(bigMin.y, origin.y);
            maxSize.x = MAX(maxSize.x, size.x);
            maxSize.y = MAX(maxSize.y, size.y);

            buffer(charsToBuffer, charBuffer, charOffsetBuffer, origin, renderSettings);
            textParsed += batchSize;
            bufferSize += charsToBuffer;
        }
        return SDL_FRect{
            bigMin.x,
            bigMin.y,
            maxSize.x,
            maxSize.y
        };
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