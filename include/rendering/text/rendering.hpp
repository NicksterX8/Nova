#ifndef TEXT_TEXT_RENDERER_INCLUDED
#define TEXT_TEXT_RENDERER_INCLUDED

#include "My/Vec.hpp"
#include "utils/vectors_and_rects.hpp"
#include "Font.hpp"
#include "formatting.hpp"
#include "rendering/utils.hpp"

namespace Text {

struct TextRenderingSettings {
    const Font* font = nullptr;
    SDL_Color color = {0,0,0,255};
    float scale = -1.0f;
};

struct TextRenderLayout {
    glm::vec2 origin;
    My::Vec<char> characters;
    My::Vec<glm::vec2> characterOffsets;
};

struct TextRenderBatch {
    TextRenderingSettings settings;

    int charBufIndex;
    int charPosBufIndex;
    int charCount;
};

struct GlyphVertex {
    glm::vec2 pos;
    
    glm::vec2 texPos;
    glm::vec2 size;

    glm::vec2 scale;
    SDL_Color color;
};

struct TextRenderer {
    using FormattingSettings = TextFormattingSettings;
    using RenderingSettings = TextRenderingSettings;

    GlModel model = {0,0,0};

    My::Vec<TextRenderBatch>* buffer;

    My::Vec<Char> charBuffer;
    My::Vec<Vec2> charPosBuffer;

    using TexCoord = glm::vec2;

    FormattingSettings defaultFormatting;
    RenderingSettings defaultRendering;
    glm::vec4 defaultColor = {0, 0, 0, 255};
    
    

    constexpr static int maxBatchSize = 1024;

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

    inline RenderResult render(const char* text, glm::vec2 pos) {
        return render(text, pos, defaultFormatting, defaultRendering);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings) {
        return render(text, strlen(text), pos, formatSettings, defaultRendering);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextRenderingSettings& renderSettings) {
        return render(text, strlen(text), pos, defaultFormatting, renderSettings);
    }

    inline RenderResult render(const char* text, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings) {
        return render(text, text ? strlen(text) : 0, pos, formatSettings, renderSettings);
    }

    RenderResult render(const char* text, int textLength, Vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings);

    // param bufferSize: size of verticesOut buffer in number of glyph vertices
    void renderBatch(const TextRenderBatch* batch, GlyphVertex* verticesOut, int bufferSize);

    // maxBatchSize: in number of characters
    void flushBuffer(const glm::mat4& transform);

    void clearBuffers() {
        // empty buffers
        this->charBuffer.size = 0;
        this->charPosBuffer.size = 0;
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