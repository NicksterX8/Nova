#include "rendering/text/rendering.hpp"

TextRenderer TextRenderer::init(const Font* defaultFont, My::Vec<TextRenderBatch>* buffer) {
    TextRenderer self;
    self.buffer = buffer;
    self.defaultFont = defaultFont;

    self.model = GlGenModel();
    self.model.bindAll();

    const static GlVertexFormat vertexFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)}, // pos
        {2, GL_FLOAT, sizeof(GLfloat)}, // texcoord
        {4, GL_UNSIGNED_BYTE, sizeof(GLubyte), true}, // color, normalized
        {2, GL_FLOAT, sizeof(GLfloat)} // scale
    });

    assert(vertexFormat.totalSize() == sizeof(GlyphVertex));

    vertexFormat.enable();
    
    GlBufferData(
        GL_ARRAY_BUFFER, 
        {maxBatchSize * 4 * sizeof(GlyphVertex), 
        nullptr, 
        GL_STREAM_DRAW});
    GlBufferData(
        GL_ELEMENT_ARRAY_BUFFER, 
        {maxBatchSize * 6 * sizeof(GLushort), 
        nullptr, 
        GL_STATIC_DRAW});

    GLushort* indices = (GLushort*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    generateQuadVertexIndices(maxBatchSize, indices);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    self.defaultColor = {0,0,0,0};
    self.defaultFormatting = FormattingSettings::Default();
    self.defaultRendering = RenderingSettings();

    return self;
}

TextRenderer::RenderResult TextRenderer::render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings, glm::vec2* outCharPositions) {
    if (!buffer) return RenderResult::BadRender();
    
    if (textLength == 0) return RenderResult::BadRender(pos);

    const Font* font = renderSettings.font ? renderSettings.font : this->defaultFont;
    if (!font || !font->face) {
        return RenderResult::BadRender(pos);
    }

    glm::vec2 scale = renderSettings.scale;
    if (scale.x < 0.0f || scale.y < 0.0f) {
        scale = this->defaultRendering.scale;
        if (scale.x < 0.0f || scale.y < 0.0f) {
            scale = {1.0f, 1.0f};
        }
    }
    Vec2 minPos = {INFINITY, INFINITY};
    Vec2 maxSize = {-INFINITY, -INFINITY};
    int textParsed = 0;

    while (textParsed < textLength) {
        int batchSize = MIN(maxBatchSize, textLength);

        auto layout = formatText(font, &text[textParsed], batchSize, formatSettings, scale);
        // format() does not account for scale so we do here.
        glm::vec2 unscaledPos = pos/scale;
        glm::vec2 scaledSize = layout.size * scale;
        glm::vec2 scaledOffset = layout.offset * scale;

        unscaledPos += formatSettings.sizeOffsetScale * scaledSize;

        
        maxSize.x = MAX(maxSize.x, scaledSize.x);
        maxSize.y = MAX(maxSize.y, scaledSize.y);
        minPos.x  = MIN(minPos.x, scaledOffset.x);
        minPos.y  = MIN(minPos.y, scaledOffset.y);

        layout.origin = unscaledPos - layout.origin;

        if (outCharPositions) {
            int regularCharIndex = 0;
            int whitespaceCharIndex = 0;
            for (int i = 0; i < batchSize; i++) {
                glm::vec2 offset;
                if (isWhitespace(text[i])) {
                    offset = layout.whitespaceCharacterOffsets[whitespaceCharIndex++];
                } else {
                    offset = layout.characterOffsets[regularCharIndex++];
                }
                outCharPositions[i] = (layout.origin + offset) * scale;
            }
        }

        if (layout.characters.size() > 0) {
            TextRenderLayout batchLayout = {
                .characterOffsets =  My::Vec<glm::vec2>(layout.characterOffsets.data(), layout.characterOffsets.size()),
                .characters = My::Vec<char>(layout.characters.data(), layout.characters.size()),
                .origin = layout.origin
            };
            auto dimensions = My::Vec<TextRenderBatch::CharacterDimensions>::WithCapacity(layout.characters.size());
            for (int i = 0; i < batchLayout.characters.size; i++) {
                dimensions.push({
                    .size = {font->size(batchLayout.characters[i])},
                    .bearing = {font->bearing(batchLayout.characters[i])}
                });
            }
            TextRenderBatch batch = {
                .font = font,
                .dimensions = dimensions,
                .layout = batchLayout,
                .colors = My::Vec<SDL_Color>::Filled(1, renderSettings.color),
                .scales = My::Vec<glm::vec2>::Filled(1, scale)
            };
            
            buffer->push(batch);
        }

        textParsed += batchSize;
    }

    glm::vec2 min = pos + minPos;
    glm::vec2 size = maxSize;

    FRect outputSize = {
        min.x,
        min.y - size.y,
        size.x,
        size.y
    };

    return RenderResult(outputSize);
}

void renderBatch(const TextRenderBatch* batch, GlyphVertex* verticesOut, int bufferSize) {
    if (!verticesOut) {
        return;
    }

    const Font* font = batch->font;
    if (!font) {
        LogError("No font in batch!");
    }
    
    const auto& layout = batch->layout;
    const auto count = layout.characters.size;
    assert(count * 4 <= bufferSize && "Batch too large!");
    for (size_t i = 0; i < count; i++) {
        char c = layout.characters[i];
        glm::vec2 offset = layout.characterOffsets[i];
        auto bearing = batch->dimensions[i].bearing;
        auto characterSize = batch->dimensions[i].size;

        GLfloat x  = layout.origin.x + (offset.x + bearing.x);
        GLfloat y  = layout.origin.y + (offset.y - (characterSize.y - bearing.y));
        GLfloat x2 = x + characterSize.x;
        GLfloat y2 = y + characterSize.y;

        //auto* coords = getTexCoords(c)->data();

        const glm::vec2 pos  = font->characters->positions[c];
        const glm::vec2 size = font->characters->sizes[c];
        const glm::vec2 texSize = font->atlasSize;

        auto tx  = (pos.x) / texSize.x;
        auto ty  = (pos.y) / texSize.y;
        auto tx2 = (pos.x + size.x) / texSize.x;
        auto ty2 = (pos.y + size.y) / texSize.y;


        SDL_Color color;
        if (batch->uniformColor()) {
            color = batch->colors[0];
        } else {
            color = batch->colors[i];
        }

        glm::vec2 scale;
        if (batch->uniformScale()) {
            scale = batch->scales[0];
        } else {
            scale = batch->scales[i];
        }

        // scale *= font->currentScale;

        const GlyphVertex vertices[] = {
            {{x,  y},  {tx, ty2}, color, scale},
            {{x,  y2}, {tx, ty}, color, scale},
            {{x2, y2}, {tx2, ty}, color, scale},
            {{x2, y},  {tx2, ty2}, color, scale}
        };
        
        memcpy(verticesOut + i * 4, vertices, sizeof(vertices));
    }
}

void flushTextBatches(MutableArrayRef<TextRenderBatch> buffer, const GlModel& model, const glm::mat4& transform, int maxBatchSize) {
    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

    unsigned int maxCharacters = 0;
    for (auto& batch : buffer) {
        int batchCharacters = batch.layout.characters.size;
        assert(batchCharacters < maxBatchSize && "Character batch too large!");
        maxCharacters = MAX(batchCharacters, maxCharacters);
    }
    const GLuint maxVertices = maxCharacters * 4;
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(GlyphVertex), NULL, GL_STREAM_DRAW);

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
        auto* batch = &buffer[i];
        Shader textShader = batch->font->shader;
        textShader.use();
        textShader.setMat4("transform", transform);
        textShader.setInt("text", batch->font->textureUnit);

        auto* vertexBuffer = (GlyphVertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        renderBatch(batch, vertexBuffer, maxBatchSize);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glDrawElements(GL_TRIANGLES, 6 * batch->layout.characters.size, GL_UNSIGNED_SHORT, NULL);
        batch->dimensions.destroy();
        batch->colors.destroy();
        batch->scales.destroy();
        batch->layout.characters.destroy();
        batch->layout.characterOffsets.destroy();
    }
    GL::logErrors();
}