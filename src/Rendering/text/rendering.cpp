#include "rendering/text/rendering.hpp"

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

struct FontRenderer {
    const Font* font;


};

TextRenderer::RenderResult TextRenderer::render(const char* text, int textLength, Vec2 position, const TextFormattingSettings& formatSettings, TextRenderingSettings renderSettings) {
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

void TextRenderer::renderBatch(const TextRenderBatch* batch, GlyphVertex* verticesOut, int bufferSize) {
    if (!verticesOut || !batch) {
        return;
    }

    const Font* font = batch->settings.font;
    if (!font) {
        LogError("No font in batch!");
    }
    
    const auto count = batch->charCount;
    assert(count <= bufferSize && "Batch too large!");
    const Char* characters = &charBuffer[batch->charBufIndex];
    const Vec2* characterPositions = &charPosBuffer[batch->charPosBufIndex];

    const SDL_Color color = batch->settings.color;
    const glm::vec2 scale = glm::vec2(batch->settings.scale);

    for (size_t i = 0; i < count; i++) {
        char c = characters[i];
        Vec2 offset = characterPositions[i];

        auto bearing = font->bearing(c);
        auto characterSize = font->size(c);

        GLfloat x  = offset.x + bearing.x;
        GLfloat y  = offset.y - (characterSize.y - bearing.y);

        const glm::vec2 texCoord  = font->characters->atlasPositions[c];

        const GlyphVertex vertex = {
            {x, y}, // position
            texCoord, 
            characterSize,
            scale,
            color
        };
        
        verticesOut[i] = vertex;
    }
}

void TextRenderer::flushBuffer(const glm::mat4& transform) {
    if (!buffer) return;

    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

    for (int i = 0; i < buffer->size; i++) {
        auto* batch = &(*buffer)[i];
        Shader textShader = batch->settings.font->shader;
        textShader.use();
        textShader.setMat4("transform", transform);
        textShader.setInt("text", batch->settings.font->textureUnit);
        textShader.setVec2("texSize", batch->settings.font->atlasSize);

        auto* vertexBuffer = (GlyphVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        renderBatch(batch, vertexBuffer, maxBatchSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glDrawArrays(GL_POINTS, 0, batch->charCount);
    }
    GL::logErrors();
    buffer->clear();
}

}