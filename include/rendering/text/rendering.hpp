#ifndef TEXT_TEXT_RENDERER_INCLUDED
#define TEXT_TEXT_RENDERER_INCLUDED

#include "My/Vec.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Font.hpp"
#include "formatting.hpp"
#include "rendering/utils.hpp"

struct TextRenderingSettings {
    const Font* font = nullptr;
    SDL_Color color = {0,0,0,255};
    glm::vec2 scale = glm::vec2(-1.0f);
};

struct TextRenderLayout {
    glm::vec2 origin;
    My::Vec<char> characters;
    My::Vec<glm::vec2> characterOffsets;
};

struct TextRenderBatch {
    const Font* font;
    struct CharacterDimensions {
        glm::vec<2, uint16_t> size;
        glm::vec<2,  int16_t> bearing;
    };
    My::Vec<CharacterDimensions> dimensions;
    My::Vec<SDL_Color> colors;
    My::Vec<glm::vec2> scales;

    // all characters use the same color
    bool uniformColor() const { return colors.size == 1; }
    // characters use the same scale
    bool uniformScale() const { return scales.size == 1; }

    TextRenderLayout layout;
};

struct GlyphVertex {
    glm::vec2 pos;
    glm::vec2 texCoord;
    SDL_Color color;
    glm::vec2 scale;
};

// param bufferSize: size of verticesOut buffer in number of glyph vertices
void renderBatch(const TextRenderBatch* batch, GlyphVertex* verticesOut, int bufferSize);
// maxBatchSize: in number of characters
void flushTextBatches(MutableArrayRef<TextRenderBatch> buffer, const GlModel& model, const glm::mat4& transform, int maxBatchSize);

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;

    const Font* defaultFont = nullptr;

    My::Vec<TextRenderBatch>* buffer;
    // index with character index found by subtracting font->firstChar from char,
    // like this: characterTexCoords['a' - font->firstChar]
    using TexCoord = glm::vec2;

    float texCoordScale = NAN;
    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;
    GlModel model = {0,0,0};
    glm::vec4 defaultColor = {0, 0, 0, 255};

    constexpr static int maxBatchSize = 5000;

    static_assert(sizeof(GlyphVertex) == sizeof(glm::vec2) + sizeof(TexCoord) + sizeof(SDL_Color) + sizeof(glm::vec2), "no struct padding");

    static TextRenderer init(const Font* defaultFont, My::Vec<TextRenderBatch>* buffer);

    struct RenderResult {
        FRect rect; // rect that text will be rendered to

        RenderResult() {}

        RenderResult(FRect outputRect)
         : rect(outputRect) {}

        static RenderResult BadRender(glm::vec2 pos = {0, 0}) {
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
        return render(text, text ? strlen(text) : 0, pos, formatSettings, renderSettings, outCharPositions);
    }

    RenderResult render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings, glm::vec2* outCharPositions = nullptr);

    void flush(const glm::mat4& transform) {
        if (!buffer) return;
        flushTextBatches(MutableArrayRef<TextRenderBatch>(buffer->data, buffer->size), model, transform, maxBatchSize);
        buffer->clear();
    }

    void destroy() {
        glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        this->model.destroy();
    }
};

#endif