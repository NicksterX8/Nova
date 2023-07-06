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
    SDL_Color color = {0,0,0,255};
    glm::vec2 scale = glm::vec2(1.0f);
    bool orderMatters = false;

    TextRenderingSettings() = default;

    TextRenderingSettings(SDL_Color color, glm::vec2 scale = glm::vec2{1.0f})
    : color(color), scale(scale) {}

    TextRenderingSettings(glm::vec2 scale) : scale(scale) {}
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return textureID;
}

FT_Face newFontFace(const char* filepath, FT_UInt height);

struct Font {
    struct FormattingSettings {
        float tabSpaces = 4.0f; // The width of a tab relative to the size of a space
        float lineHeightScale = 1.0f; // Line height scalar (space between lines, 1.0 is one font height's distance)
    };

    struct AtlasCharacterData {
        glm::vec<2, uint16_t>* positions = nullptr;
        glm::vec<2, uint16_t>* sizes = nullptr;
        glm::vec<2,  int16_t>* bearings = nullptr;
        uint16_t* advances = nullptr;
    };

    FT_Face face = nullptr;
    AtlasCharacterData characters = {};

    GLuint atlasTexture = 0;
    glm::ivec2 atlasSize = {0, 0};

    FormattingSettings formatting;

    FT_UInt baseHeight; // the height of the font face before any scaling. The true height is this * scale
    float currentScale;

    char firstChar = -1;
    char lastChar = -1;
    bool usingSDFs = false;

    Font() = default;

    Font(const char* fontfile, FT_UInt baseHeight, bool useSDFs, char _firstChar, char _lastChar, float scale, FormattingSettings formatting, TextureUnit textureUnit);

    void unload();

    // true height
    float height() const {
        //assert(face->height >> 6 == (unsigned int)round(baseHeight * scale));
        //return baseHeight * scale;
        return face->height >> 6;
    }

    float tabPixels() const {
        return (advance(' ') >> 6) * formatting.tabSpaces;
    }

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

    // right shift 6 to get advance in true size
    unsigned int advance(char c) const {
        assert(c >= firstChar && c <= lastChar && "character not available in font!");
        return characters.advances[c - firstChar];
    }

    void load(FT_UInt height, GLuint textureUnit, bool useSDFs = false);
};

struct CharacterLayoutData {
    glm::vec2 size = {0, 0};
    glm::vec2 origin = {0, 0};
    llvm::SmallVector<char> characters;
    llvm::SmallVector<glm::vec2> characterOffsets;
};

struct TextRenderBatch {
    TextRenderingSettings settings;
    CharacterLayoutData layout;
};

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;

    Font* font = nullptr;
    llvm::SmallVector<TextRenderBatch, 0> buffer;
    std::array<glm::vec2, 4>* characterTexCoords = nullptr;
    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;
    GlModel model;
    glm::vec4 defaultColor = {0, 0, 0, 255};

    constexpr static int maxBatchSize = 5000;

    // Returns a pointer to the set of 4 vectors that make up the character's texture quad on the font atlas
    std::array<glm::vec2, 4>* getTexCoords(char c) {
        return &characterTexCoords[c - font->firstChar];
    }

    void calculateCharacterTexCoords(std::array<glm::vec2, 4>* texCoords) {
        glm::vec2 textureSize = font->atlasSize;
        const auto* characterPositions = font->characters.positions;
        const auto* characterSizes     = font->characters.sizes;

        auto numChars = font->lastChar - font->firstChar;
        for (int i = 0; i < numChars; i++) {
            glm::vec2* coords = texCoords[i].data();
            const glm::ivec2 pos  = characterPositions[i];
            const glm::ivec2 size = characterSizes[i];

            auto tx  = (GLfloat)(pos.x + 0.01f) / textureSize.x;
            auto ty  = (GLfloat)(pos.y + 0.01f) / textureSize.y;
            auto tx2 = (GLfloat)(pos.x + size.x - 0.01f) / textureSize.x;
            auto ty2 = (GLfloat)(pos.y + size.y - 0.01f) / textureSize.y;

            coords[0] = {tx , ty2};
            coords[1] = {tx , ty };
            coords[2] = {tx2, ty };
            coords[3] = {tx2, ty2};
        }
    }

    static TextRenderer init(Font* font) {
        TextRenderer self;
        self.font = font;
        self.defaultColor = {0,0,0,0};

        self.model = GlGenModel();
        self.model.bindAll();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, maxBatchSize * 6 * sizeof(GLushort), NULL, GL_STATIC_DRAW);
        GLushort* indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        generateQuadVertexIndices(maxBatchSize, indices);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        // position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
        // tex coord
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

        // unbind buffers just in case
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // allocate buffers
        self.defaultFormatting = FormattingSettings::Default();

        self.characterTexCoords = Alloc<std::array<glm::vec2, 4>>(font->lastChar - font->firstChar + 1); // last char is inclusive so add 1
        self.calculateCharacterTexCoords(self.characterTexCoords);

        return self;
    }

    float lineHeight(float scale) {
       return scale * font->height();
    }

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

    // TODO: fix bug with maxWidth scaling thing

    // This is where the magic happens
    CharacterLayoutData format(const char* text, int textLength, FormattingSettings settings) {
        // current x, current y
        float cx = 0.0f,cy = 0.0f;
        // max X, max Y
        float mx = 0.0f,my = 0.0f;
        int lineLength = 0;

        CharacterLayoutData layout = {};

        for (int i = 0; i < textLength; i++) {
            char c = text[i];
            float charPixelsAdvance = 0.0f;
            int sizeX = 0;
            bool whitespaceChar = false;
            if (c == '\n') {
                whitespaceChar = true;
                goto newline;
            } else if (c == '\t') {
                charPixelsAdvance = font->tabPixels();
                whitespaceChar = true;
                goto ready;
            } else if (c < font->firstChar || c > font->lastChar) {
                // turn unsupported characters into hashtags
                c = '#';
            }
            charPixelsAdvance = (float)(font->advance(c) >> 6);
        ready:
            if (cx + charPixelsAdvance + sizeX >= settings.maxWidth) {
            newline:
                if (settings.align.horizontal != HoriAlignment::Left) {
                    // shift whole line over to correct alignment location (center or right)
                    float lineMovement = 0.0f;
                    if (settings.align.horizontal == HoriAlignment::Right) {
                        lineMovement = cx;
                    } else if (settings.align.horizontal == HoriAlignment::Center) {
                        lineMovement = cx/2.0f;
                    }
                    // shift the lineLength most recent characters back by lineMovement
                    for (int j = 0; j < lineLength; j++) {
                        // should be impossible for layout.characters.size to be less than lineLength
                        layout.characterOffsets[layout.characters.size() - j - 1].x -= lineMovement;
                    }
                    // reset line
                    lineLength = 0;
                }

                cy -= font->formatting.lineHeightScale * font->height();
                mx = MAX(MAX(mx, cx), settings.maxWidth);
                cx = 0.0f;
            }

            if (!whitespaceChar) {
                layout.characters.push_back(c);
                layout.characterOffsets.push_back(glm::vec2(cx, cy));
                lineLength++;
            }

            cx += charPixelsAdvance;
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
                layout.characterOffsets[layout.characters.size() - j - 1].x -= lineMovement;
            }
            // reset line
            lineLength = 0;
        }

        glm::vec2 origin = {0,0};

        mx = cx > mx ? cx : mx;
        my = cy < my ? cy : my;

        float line = lineHeight(1.0f); // scaled later, so no need to scale here
        float ascender = (float)(font->face->ascender >> 6);
        float descender = (float)(font->face->descender >> 6);
        switch (settings.align.vertical) {
        case VertAlignment::Top:
            origin.y += line + ascender;
            break;
        case VertAlignment::Middle:
            origin.y += my/2.0f;
            origin.y += line/2.0f;
            break;
        case VertAlignment::Bottom:
            origin.y += my;
            origin.y += descender;
            break;
        default:
            break;
        }

        layout.size = {mx, -my + (font->face->height >> 6)};
        layout.origin = origin;
        return layout;
    }

private:
    void renderBatch(const TextRenderBatch* batch, GLfloat* verticesOut) {
        const auto& layout = batch->layout;
        const auto settings = batch->settings;
        const auto count = layout.characters.size();
        assert(count < maxBatchSize && "Batch too large!");
        for (size_t i = 0; i < count; i++) {
            char c = layout.characters[i];
            glm::vec2 offset = layout.characterOffsets[i];
            auto bearing = font->bearing(c);
            auto size = font->size(c);


            GLfloat x  = layout.origin.x + (offset.x + bearing.x);
            GLfloat y  = layout.origin.y + (offset.y - (size.y - bearing.y));
            GLfloat x2 = x + size.x;
            GLfloat y2 = y + size.y;

            glm::vec2* coords = getTexCoords(c)->data();
            // update VBO for each character
            // rendering glyph texture over quad
            const GLfloat vertices[4 * 4] = {
                x,  y,  coords[0].x, coords[0].y,
                x,  y2, coords[1].x, coords[1].y,
                x2, y2, coords[2].x, coords[2].y,
                x2, y,  coords[3].x, coords[3].y
            };
            
        
            // add to vertex buffer
            /*
            const GLfloat positions[4][2] = {
                {x,  y},
                {x,  y2},
                {x2, y2},
                {x2, y}
            }; 
            memcpy((char*)vertexBuffer + bufferPositionStart + (i + bufferSize) * sizeof(positions), positions, sizeof(positions));
            const float* texCoords = (float*)characterTexCoords[c - font->firstChar].data();
            memcpy((char*)vertexBuffer + bufferTexCoordStart + (i + bufferSize) * 4 * sizeof(glm::vec2), texCoords, 4 * sizeof(glm::vec2));
            */
            memcpy(verticesOut + i * 16, vertices, sizeof(vertices));
        }
    }
public:

    inline FRect render(const char* text, glm::vec2 pos) {
        return render(text, pos, defaultFormatting, defaultRendering);
    }

    inline FRect render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings) {
        return render(text, strlen(text), pos, formatSettings, renderSettings);
    }

    FRect render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings) {
        if (!this->font || !this->font->face) {
            return {0, 0, 0, 0};
        }

        Vec2 bigMin = {pos.x, pos.y};
        Vec2 maxSize = {0, 0};
        int textParsed = 0;
        while (textParsed < textLength) {
            int batchSize = MIN(maxBatchSize, textLength);

            auto layout = format(&text[textParsed], batchSize, formatSettings);
            int charsToBuffer = layout.characters.size(); // not all characters need to actually be buffered (space, tab, etc)
            layout.origin = pos/renderSettings.scale - layout.origin;
            maxSize.x = MAX(maxSize.x, layout.size.x);
            maxSize.y = MAX(maxSize.y, layout.size.y);

            TextRenderBatch batch = {
                .layout = layout,
                .settings = renderSettings
            };
            buffer.push_back(batch);

            textParsed += batchSize;
        }

        // size will be smaller after scaling. format() does not account for it so we do here.
        maxSize *= renderSettings.scale;

        return SDL_FRect{
            bigMin.x,
            bigMin.y,
            maxSize.x,
            maxSize.y
        };
    }

    // texture will be bound on the active texture
    void flush(Shader textShader, glm::mat4 transform, GLuint textureUnit) {
        glBindVertexArray(model.vao);
        glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, font->atlasTexture);
        textShader.use();

        unsigned int maxCharacters = 0;
        for (auto& batch : buffer) {
            int batchCharacters = batch.layout.characters.size();
            assert(batchCharacters < maxBatchSize && "Character batch too large!");
            maxCharacters = MAX(batchCharacters, maxCharacters);
        }

        const GLuint maxVertices = maxCharacters * 4;
        const GLuint maxFloats = maxVertices * 4;

        glBufferData(GL_ARRAY_BUFFER, maxFloats * sizeof(GLfloat), NULL, GL_STREAM_DRAW);

        // TODO: For merging batches to help performance
            /*
            if (!buffer.empty()) {
                auto& lastBatch = buffer.back();
                auto lastBatchSettings = lastBatch.settings;
                if (!lastBatchSettings.orderMatters && lastBatchSettings.color == renderSettings.color && lastBatchSettings.scale == renderSettings.scale) {
                    // share batch
                    lastBatch.layout.characters.append(layout.characters);
                    lastBatch.layout.characterOffsets.append(layout.characterOffsets);
                    lastBatch.layout.size 
                }
                
            }
            */

        for (int i = 0; i < buffer.size(); i++) {
            const auto* batch = &buffer[i];
            textShader.setVec3("textColor", colorConvert(batch->settings.color));
            textShader.setMat4("transform", glm::scale(transform, glm::vec3(batch->settings.scale, 1.0f)));
            auto* vertexBuffer = (GLfloat*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            renderBatch(batch, vertexBuffer);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glDrawElements(GL_TRIANGLES, 6 * batch->layout.characters.size(), GL_UNSIGNED_SHORT, NULL);
        }
        buffer.clear();
    }

    FRect renderStringBufferAsLinesReverse(const My::StringBuffer& strings, int maxLines, glm::vec2 pos, My::Vec<SDL_Color> colors, glm::vec2 scale, FormattingSettings formatSettings) {
        FRect maxRect = {pos.x,pos.y,0,0};
        glm::vec2 offset = {0, 0};
        int i = 0;
        strings.forEachStrReverse([&](char* str) -> bool{
            if (i >= maxLines) return true;
            FRect rect = render(str, pos + offset, formatSettings, TextRenderingSettings{colors[colors.size - 1 - i], scale});
            //unionRectInPlace(&maxRect, &rect);
            SDL_UnionFRect(&maxRect, &rect, &maxRect);
            offset.y += rect.h;
            // do line break
            offset.y += font->formatting.lineHeightScale * (font->face->height >> 6) * scale.y;
            i++;
            return false;
        });

        return maxRect;
    }

    void destroy() {
        glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        this->model.destroy();
        Free(characterTexCoords);
    }
};


#endif 