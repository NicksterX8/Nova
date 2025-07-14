#ifndef TEXT_TEXT_RENDERER_INCLUDED
#define TEXT_TEXT_RENDERER_INCLUDED

#include "My/Vec.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Font.hpp"
#include "formatting.hpp"
#include "rendering/utils.hpp"
#include "ADT/TinyValVector.hpp"

namespace Text {


struct TextRenderingSettings {
    const Font* font = nullptr;
    SDL_Color color = {0, 0, 0, 255};
    float scale = -1.0f;
};

using TextHeight = float;

struct TextRenderBatch {
    TextRenderingSettings settings;

    int charBufIndex = -1;
    int charPosBufIndex = -1;
    int charColorBufIndex = -1;
    int charCount = -1;
    TextHeight height = NAN;
};

struct GlyphVertex {
    glm::vec2 pos;
    glm::vec2 texPos;
    GLfloat fontID;
    glm::vec2 size;

    glm::vec2 scale;
    SDL_Color color;
};

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;

    GlModel model = {0,0,0};

    My::Vec<TextRenderBatch> buffer;

    My::Vec<Char> charBuffer;
    My::Vec<Vec2> charPosBuffer;
    My::Vec<SDL_Color> charColorBuffer;

    using TexCoord = glm::vec2;

    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;
    glm::vec4 defaultColor = {0, 0, 0, 255};
    
    glm::mat4 transform;

    float heightIncrementer = 0.0f;
    static constexpr float heightIncrement = 0.0001f;

    constexpr static int maxBatchSize = 1024;

    void init(const Font* defaultFont);

    struct RenderResult {
        FRect rect; // rect that text will be rendered to

        RenderResult() : rect({NAN, NAN, NAN, NAN}) {}

        RenderResult(FRect outputRect)
         : rect(outputRect) {}

        static RenderResult BadRender(glm::vec2 pos = {0, 0}) {
            return RenderResult(FRect{pos.x, pos.y, 0, 0});
        }
    };

    inline RenderResult render(const char* text, glm::vec2 pos, TextHeight height) {
        return render(text, pos, defaultFormatting, defaultRendering, height);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, TextHeight height) {
        return render(text, strlen(text), pos, formatSettings, defaultRendering, height);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextRenderingSettings& renderSettings, TextHeight height) {
        return render(text, strlen(text), pos, defaultFormatting, renderSettings, height);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings, TextHeight height) {
        return render(text, text ? strlen(text) : 0, pos, formatSettings, renderSettings, height);
    }

    RenderResult render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings, TextHeight height = 0, ArrayRef<SDL_Color> colors = {});

    RenderResult renderColored(const char* text, int textLength, ArrayRef<SDL_Color> colors,
        glm::vec2 pos, const TextFormattingSettings& formatSettings, 
        RenderingSettings renderSettings, TextHeight height = 0)
    {
        return render(text, textLength, pos, formatSettings, renderSettings, height, colors);
    }

    // param bufferSize: size of verticesOut buffer in number of glyph vertices
    void renderBatchMonocolor(const TextRenderBatch* batch, int batchIndex, int count, GlyphVertex* verticesOut, int bufferSize);

    void renderBatchMulticolor(const TextRenderBatch* batch, int batchIndex, int count, GlyphVertex* verticesOut, int bufferSize);

    int flushBufferType(bool sdfs);

    // maxBatchSize: in number of characters
    void flushBuffer();

    void clearBuffers() {
        // empty buffers
        this->charBuffer.size = 0;
        this->charPosBuffer.size = 0;
        this->charColorBuffer.size = 0;
    }

    void destroy() {
        glBindBuffer(GL_ARRAY_BUFFER, this->model.vbo);
        glUnmapBuffer(GL_ARRAY_BUFFER);
        this->model.destroy();
    }
};

}

using Text::TextRenderer;
using Text::TextRenderBatch;
using Text::TextRenderingSettings;

#endif