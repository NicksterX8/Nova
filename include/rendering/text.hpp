#ifndef RENDERING_TEXT_INCLUDED
#define RENDERING_TEXT_INCLUDED

#include <optional>
#include <glm/vec2.hpp>
#include "../sdl_gl.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Shader.hpp"
#include "utils.hpp"
#include "TexturePacker.hpp"
#include "My/String.hpp"

#include "global.hpp"

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
    AtlasCharacterData* characters = {};

    FT_UInt _height = 0;
    GLuint atlasTexture = 0;
    glm::ivec2 atlasSize = {0, 0};

    FormattingSettings formatting;

    // in the case that char is signed, its impossible to have an exclusive range end while also supporting
    //  chars 0-127, so we use inclusive end range
    char firstChar=0,lastChar=-1;
    bool usingSDFs = false;
    TextureUnit textureUnit = TextureUnit::Null;

    FT_UInt baseHeight = 0; // the height of the font face before any scaling. The true height is this * scale

    float currentScale = 0.0f;

    Font() = default;

    Font(const char* fontfile, FT_UInt baseHeight, bool useSDFs, char _firstChar, char _lastChar, float scale, FormattingSettings formatting, TextureUnit textureUnit);

    void load(FT_UInt height, bool useSDFs = false);

    bool loaded() const {
        return face != nullptr;
    }

    void unload();
    
    // get the number of characters supported by this font structure (not the underlying font face, that probably supports a lot more)
    int numChars() const {
        return (int)lastChar - (int)firstChar + 1;
    }

    FT_UInt linePixelSpacing() const {
        return formatting.lineSpacing * height();
    }

    FT_UInt ascender() const {
        assert(face);
        return face->size->metrics.ascender >> 6;
    }

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

    void scale(float scale) {
        if (scale == currentScale) return;
        load(baseHeight * scale, usingSDFs);
    }
};

/* FORMATTING */

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

struct TextFormattingSettings {
    TextAlignment align = TextAlignment::TopLeft;
    float maxWidth = INFINITY;
    float maxHeight = INFINITY;
    float sizeOffsetScale = 0.0f;
    bool wrapOnWhitespace = true;

    TextFormattingSettings() = default;

    TextFormattingSettings(
        TextAlignment alignment, float maxWidth = INFINITY, float maxHeight = INFINITY)
        : align(alignment), maxWidth(maxWidth), maxHeight(maxHeight) {}

private:
    using Self = TextFormattingSettings;
public:

    static Self Default() {
        return Self();
    }
};

struct CharacterLayoutData {
    glm::vec2 size = {0, 0}; // size of the body of output text
    glm::vec2 origin = {0, 0};
    glm::vec2 offset = {0, 0}; // offset to original position
    llvm::SmallVector<char> characters;
    llvm::SmallVector<glm::vec2> characterOffsets;
    llvm::SmallVector<char> whitespaceCharacters;
    llvm::SmallVector<glm::vec2> whitespaceCharacterOffsets;
};

CharacterLayoutData formatText(const Font* font, const char* text, int textLength, TextFormattingSettings settings, glm::vec2 scale);

// handle horizontal alignment by shifting line to correct position on the x axis
inline void alignLineHorizontally(MutableArrayRef<glm::vec2> offsets, float originOffset, HoriAlignment align) {
    if (align != HoriAlignment::Left) {
        // shift whole line over to correct alignment location (center or right)
        float lineMovement = 0.0f;
        if (align == HoriAlignment::Right) {
            lineMovement = 1.0f * originOffset;
        } else if (align == HoriAlignment::Center) {
            lineMovement = 0.5f * originOffset;
        }
        for (int j = 0; j < offsets.size(); j++) {
            offsets[j].x -= lineMovement;
        }
    }
}

/* RENDERING */

struct TextRenderingSettings {
    const Font* font = nullptr;;
    SDL_Color color = {0,0,0,255};
    glm::vec2 scale = glm::vec2(1.0f);
    bool orderMatters = false;

    TextRenderingSettings() = default;

    TextRenderingSettings(SDL_Color color, glm::vec2 scale = glm::vec2{1.0f})
    : color(color), scale(scale) {}

    TextRenderingSettings(glm::vec2 scale) : scale(scale) {}
};

struct TextRenderLayout {
    glm::vec2 origin;
    llvm::SmallVector<char> characters;
    llvm::SmallVector<glm::vec2> characterOffsets;
};

struct TextRenderBatch {
    const Font* font;
    llvm::SmallVector<SDL_Color> colors;
    llvm::SmallVector<glm::vec2> scales;

    // all characters use the same color
    bool uniformColor() const { return colors.size() == 1; }
    // characters use the same scale
    bool uniformScale() const { return scales.size() == 1; }

    TextRenderLayout layout;
};

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;
private:
    const Font* font = nullptr;
public:
    llvm::SmallVector<TextRenderBatch, 0> buffer;
    // index with character index found by subtracting font->firstChar from char,
    // like this: characterTexCoords['a' - font->firstChar]
    using TexCoord = glm::vec2;
    std::array<TexCoord, 4>* characterTexCoords = nullptr; // atlas 
    float texCoordScale = NAN;
    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;
    GlModel model = {0,0,0};
    glm::vec4 defaultColor = {0, 0, 0, 255};

    constexpr static int maxBatchSize = 5000;

    struct Vertex {
        glm::vec2 pos;
        TexCoord texCoord;
        SDL_Color color;
        glm::vec2 scale;
    };
    static_assert(sizeof(Vertex) == sizeof(glm::vec2) + sizeof(TexCoord) + sizeof(SDL_Color) + sizeof(glm::vec2), "no struct padding");

    // Returns a pointer to the set of 4 vectors that make up the character's texture quad on the font atlas
    std::array<TexCoord, 4>* getTexCoords(char c) const {
        return &characterTexCoords[c - font->firstChar];
    }

    void calculateCharacterTexCoords() {
        if (!font) return;

        glm::vec2 texSize = font->atlasSize;

        const auto* characterPositions = font->characters->positions;
        const auto* characterSizes     = font->characters->sizes;

        auto numChars = font->numChars();
        for (int i = 0; i < numChars; i++) {
            char c = font->firstChar + (char)i;
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

        texCoordScale = font->currentScale;
    }

    static TextRenderer init(Font* font);

    const Font* getFont() const {
        return font;
    }

    void setFont(const Font* newFont) {
        if (newFont == font) return;

        font = newFont;

        if (font) {
            characterTexCoords = Realloc(characterTexCoords, font->numChars());
            calculateCharacterTexCoords();
            texCoordScale = font->currentScale;
        }
    }

    bool canDraw() const {
        return font != nullptr && font->face != nullptr;
    }

    float lineHeight() const {
        return font ? font->height() : NAN;
    }

    struct RenderResult {
        FRect rect; // rect that text will be rendered to

        RenderResult() {}

        RenderResult(FRect outputRect)
         : rect(outputRect) {}

        static RenderResult BadRender(glm::vec2 pos) {
            return RenderResult(FRect{pos.x, pos.y, 0, 0});
        }
    };

    inline RenderResult render(const char* text, glm::vec2 pos, glm::vec2* outCharPositions = nullptr) {
        return render(text, pos, defaultFormatting, defaultRendering, outCharPositions);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, glm::vec2* outCharPositions = nullptr) {
        return render(text, strlen(text), pos, formatSettings, defaultRendering, outCharPositions);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextRenderingSettings& renderSettings, glm::vec2* outCharPositions = nullptr) {
        return render(text, strlen(text), pos, defaultFormatting, renderSettings, outCharPositions);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings, glm::vec2* outCharPositions = nullptr) {
        return render(text, strlen(text), pos, formatSettings, renderSettings, outCharPositions);
    }

    RenderResult render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings, glm::vec2* outCharPositions = nullptr);

private:
    void renderBatch(const TextRenderBatch* batch, Vertex* verticesOut) const;
public:

    void flush(Shader textShader, glm::mat4 transform);

    FRect renderStringBufferAsLinesReverse(const My::StringBuffer& strings, int maxLines, glm::vec2 pos, My::Vec<SDL_Color> colors, glm::vec2 scale, FormattingSettings formatSettings) {
        if (!canDraw()) return {pos.x,pos.y,0,0};
        
        FRect maxRect = {pos.x,pos.y,0,0};
        glm::vec2 offset = {0, 0};
        int i = 0;
        strings.forEachStrReverse([&](char* str) -> bool{
            if (i >= maxLines) return true;
            auto result = render(str, pos + offset, formatSettings, TextRenderingSettings{colors[colors.size - 1 - i], scale});
            SDL_UnionFRect(&maxRect, &result.rect, &maxRect);
            auto outputHeight = result.rect.h;
            formatSettings.maxHeight -= outputHeight * scale.y;
            offset.y += outputHeight;
            // do line break
            offset.y += font->linePixelSpacing() * scale.y;
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