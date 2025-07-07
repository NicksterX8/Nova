#include "rendering/text/rendering.hpp"
#include "rendering/shaders.hpp"

namespace Text {

TextRenderer TextRenderer::init(const Font* defaultFont, My::Vec<TextRenderBatch>* buffer) {
    TextRenderer self;
    self.buffer = buffer;
    self.charBuffer = My::Vec<Char>::WithCapacity(256);
    self.charPosBuffer = My::Vec<Vec2>::WithCapacity(256);

    self.model = GlGenModel();
    self.model.bindAll();

    const static GlVertexFormat vertexFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)}, // pos
        {2, GL_FLOAT, sizeof(GLfloat)}, // texcoord
        {1, GL_FLOAT, sizeof(GLfloat)},
        {2, GL_FLOAT, sizeof(GLfloat)}, // size
        {2, GL_FLOAT, sizeof(GLfloat)}, // scale
        {4, GL_UNSIGNED_BYTE, sizeof(GLubyte), true} // color, normalized
    });

    assert(vertexFormat.totalSize() == sizeof(GlyphVertex));

    vertexFormat.enable();
    
    GlBufferData(
        GL_ARRAY_BUFFER, 
        {maxBatchSize * sizeof(GlyphVertex), 
        nullptr, 
        GL_STREAM_DRAW});

    self.defaultColor = {0,0,0,0};
    self.defaultFormatting = FormattingSettings::Default();
    self.defaultRendering = RenderingSettings{
        .font = defaultFont,
        .scale = 1.0f
    };

    return self;
}

TextRenderer::RenderResult TextRenderer::render(const char* text, int textLength, Vec2 position, const TextFormattingSettings& formatSettings, TextRenderingSettings renderSettings) {
    if (!text || textLength == 0) return RenderResult::BadRender(position);
    
    if (!renderSettings.font) {
        if (!defaultRendering.font) {
            LogError("No font to use for render!");
            return RenderResult::BadRender(position);
        }
        renderSettings.font = defaultRendering.font;
    }

    if (renderSettings.scale < 0.0f) {
        renderSettings.scale = defaultRendering.scale;
    }

    int charBufIndex = charBuffer.size;
    int charPosBufIndex = charPosBuffer.size;
    auto formatResult = formatText(
        renderSettings.font, text, textLength,
        formatSettings, position, renderSettings.scale,
        &charBuffer, &charPosBuffer);

    if (formatResult.visibleCharCount == 0) return *boxAsRect(&formatResult.boundingBox);

    const TextRenderBatch renderBatch = {
        .settings = renderSettings,
        .charBufIndex = charBufIndex,
        .charPosBufIndex = charPosBufIndex,
        .charCount = formatResult.visibleCharCount
    };
    buffer->push(renderBatch);
    return {
        *boxAsRect(&formatResult.boundingBox)
    };
}

void TextRenderer::renderBatch(const TextRenderBatch* batch, int batchIndex, int count, GlyphVertex* verticesOut, int bufferSize) {
    if (!verticesOut || !batch || batch->charCount == 0) {
        return;
    }

    const Font* font = batch->settings.font;
    if (!font) {
        LogError("No font in batch!");
    }
    
    assert(batchIndex + count <= batch->charCount && "Count beyond batch size");
    assert(count <= bufferSize && "Batch too large!");
    const Char* characters = &charBuffer[batch->charBufIndex];
    const Vec2* characterPositions = &charPosBuffer[batch->charPosBufIndex];

    const SDL_Color color = batch->settings.color;
    const glm::vec2 scale = glm::vec2(batch->settings.scale);
    const GLfloat fontID = (float)batch->settings.font->id;

    for (size_t i = 0; i < count; i++) {
        int charIndex = i + batchIndex;
        char c = characters[charIndex];
        Vec2 offset = characterPositions[charIndex];

        auto bearing = font->bearing(c);
        auto characterSize = font->size(c);

        GLfloat x  = offset.x + bearing.x;
        GLfloat y  = offset.y - (characterSize.y - bearing.y);

        const glm::vec2 texCoord  = font->characters->atlasPositions[c];

        const GlyphVertex vertex = {
            {x, y}, // position
            texCoord, 
            fontID,
            characterSize,
            scale,
            color
        };
        
        verticesOut[i] = vertex;
    }
}

int TextRenderer::flushBufferType(bool sdfs) {
    Shader shader;
    if (sdfs) {
        shader = getShader(Shaders::SDF);
    } else {
        shader = getShader(Shaders::Text);
    }
    shader.use();

    auto* vertexBuffer = (GlyphVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    int totalChars = 0;
    int bufferedChars = 0;
    for (int i = 0; i < buffer->size; i++) {
        auto* batch = &(*buffer)[i];
        if (batch->settings.font->usingSDFs == sdfs) {
            int batchCharsRendered = 0;
            while (batchCharsRendered < batch->charCount) {
                int charsToRender = std::min(maxBatchSize - bufferedChars, batch->charCount - batchCharsRendered); 
                renderBatch(batch, batchCharsRendered, charsToRender, vertexBuffer + bufferedChars, maxBatchSize);
                bufferedChars += charsToRender;
                if (bufferedChars == maxBatchSize) {
                    glUnmapBuffer(GL_ARRAY_BUFFER);
                    glDrawArrays(GL_POINTS, 0, maxBatchSize);
                    vertexBuffer = (GlyphVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                    totalChars += maxBatchSize;
                    bufferedChars = 0;
                }
                batchCharsRendered += charsToRender;
            }
        }
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    if (bufferedChars > 0) {
        glDrawArrays(GL_POINTS, 0, bufferedChars);
        totalChars += bufferedChars;
    }
    return totalChars;
}

void TextRenderer::flushBuffer() {
    if (!buffer || buffer->empty()) return;

    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
    flushBufferType(true);
    flushBufferType(false);
    GL::logErrors();
    buffer->clear();
}

}