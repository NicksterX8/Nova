#include "rendering/text.hpp"
#include "rendering/textures.hpp"

FT_Library freetype;

FT_Face newFontFace(const char* filepath, FT_UInt height) {
    FT_Error err = 0;
    FT_Face face = nullptr;
    err = FT_New_Face(freetype, filepath, 0, &face);
    if (err) {
        LogError("Failed to load font face. Filepath: %s. Error: %s", filepath, FT_Error_String(err));
        return nullptr;
    }

    err = FT_Set_Pixel_Sizes(face, 0, height);
    if (err) {
        LogError("Failed to set font face pixel size. FT_Error: %s", FT_Error_String(err));
    }

    return face;
}

const TextAlignment TextAlignment::TopLeft      = {HoriAlignment::Left,   VertAlignment::Top};
const TextAlignment TextAlignment::TopCenter    = {HoriAlignment::Center, VertAlignment::Top};
const TextAlignment TextAlignment::TopRight     = {HoriAlignment::Right,  VertAlignment::Top};
const TextAlignment TextAlignment::MiddleLeft   = {HoriAlignment::Left,   VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleCenter = {HoriAlignment::Center, VertAlignment::Middle};
const TextAlignment TextAlignment::MiddleRight  = {HoriAlignment::Right,  VertAlignment::Middle};
const TextAlignment TextAlignment::BottomLeft   = {HoriAlignment::Left,   VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomCenter = {HoriAlignment::Center, VertAlignment::Bottom};
const TextAlignment TextAlignment::BottomRight  = {HoriAlignment::Right,  VertAlignment::Bottom};

Font* newFont(const char* fontfile, FT_UInt baseHeight, bool useSDFs, char _firstChar, char _lastChar, float scale, Font::FormattingSettings formatting, TextureUnit textureUnit, Shader shader) {
    Font* f = new Font();
    
    f->baseHeight = baseHeight;

    FT_UInt _height = baseHeight * scale;
    f->currentScale = scale;

    f->formatting = formatting;
    f->textureUnit = textureUnit;

    if (_lastChar > 127 || _lastChar < 0 || _firstChar > 127 || _firstChar < 0) {
        LogError("Can't load font characters higher than 127 or lower than 0!");
        if (_lastChar > 127) _lastChar = 127;
        if (_firstChar > 127) _firstChar = 127;
        if (_lastChar < 0) _lastChar = 0;
        if (_firstChar < 0) _firstChar = 0;
    }
    
    f->firstChar = _firstChar;
    f->lastChar = _lastChar;

    f->usingSDFs = useSDFs;

    FT_Error err = 0;
    err = FT_New_Face(freetype, fontfile, 0, &f->face);
    if (err || !f->face) {
        LogError("Failed to load font face. Filepath: %s. Error: %s", fontfile, FT_Error_String(err));
        delete f;
        return nullptr;
    }

    char numChars = f->lastChar - f->firstChar;
    if (numChars > 0) {
        f->characters = Alloc<Font::AtlasCharacterData>();
        f->load(_height, shader, useSDFs);
    }

    return f;
}

void Font::load(FT_UInt pixelHeight, Shader shader, bool useSDFs) {
    this->currentScale = (float)pixelHeight / (float)baseHeight;

    this->usingSDFs = useSDFs;
    this->shader = shader;

    FT_Error err = FT_Set_Pixel_Sizes(face, 0, pixelHeight);

    this->_height = pixelHeight;
    
    if (err) {
        LogError("Failed to set font face pixel size. FT_Error: %s", FT_Error_String(err));
        return;
    }

    int nChars = numChars();
    if (nChars <= 0) return;

    constexpr int pixelSize = 1; // Character glyphs use 1 byte pixels, black and white
    glPixelStorei(GL_UNPACK_ALIGNMENT, pixelSize); // disable byte-alignment restriction

    memset(characters, 0, sizeof(decltype(*characters)));

    if (this->face) {
        FT_GlyphSlot slot = face->glyph;


        FT_Error error;

        auto textureAtlas = makeTexturePackingAtlas(pixelSize, nChars);
    
        for (size_t i = 0; i < nChars; i++) {
            char c = firstChar + (char)i;
            // load character glyph 
            if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER))) {
                LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                continue;
            }

            if (c != ' ') {
                auto mode = useSDFs ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL;
                if (usingSDFs) {
                    if ((error = FT_Render_Glyph(slot, mode))) {
                        LogError("Failed to load sdf glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                        continue;
                    }
                }
            }

            assert(slot->advance.x >= 0 && slot->advance.x >> 6 <= UINT16_MAX);
            characters->advances[c]  = (uint16_t)(slot->advance.x >> 6);
            characters->bearings[c]  = glm::vec<2,  int16_t>{slot->bitmap_left, slot->bitmap_top};
            characters->sizes[c]     = glm::vec<2, uint16_t>{slot->bitmap.width, slot->bitmap.rows};
            characters->positions[c] = packTexture(&textureAtlas, Texture{slot->bitmap.buffer, characters->sizes[c], pixelSize}, {1, 1});
        }

        characters->advances['\t'] = characters->advances[' '] * formatting.tabSpaces;

        Texture packedTexture = doneTexturePackingAtlas(&textureAtlas);
        this->atlasSize = packedTexture.size;

        glActiveTexture(GL_TEXTURE0 + textureUnit);
        constexpr GLint regularMinFilter = GL_LINEAR;
        constexpr GLint regularMagFilter = GL_LINEAR;
        constexpr GLint sdfMinFilter     = GL_LINEAR;
        constexpr GLint sdfMagFilter     = GL_LINEAR;
        GLint minFilter = usingSDFs ? sdfMinFilter : regularMinFilter;
        GLint magFilter = usingSDFs ? sdfMagFilter : regularMagFilter;
        if (this->atlasTexture) {
            glDeleteTextures(1, &this->atlasTexture);
            this->atlasTexture = 0;
        }
        this->atlasTexture = loadFontAtlasTexture(packedTexture, minFilter, magFilter);
        freeTexture(packedTexture);
    }
    GL::logErrors();
    
    if (!this->atlasTexture) {
        LogError("Font atlas texture failed to generate!");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

void Font::unload() {

    if (!face || !face->family_name) return;
    LogInfo("Unloading font %s-%s", face->family_name, face->style_name);
    FT_Done_Face(face); face = nullptr;
    Free(characters);
    glDeleteTextures(1, &atlasTexture);
}

CharacterLayoutData formatText(const Font* font, const char* text, int textLength, TextFormattingSettings settings, glm::vec2 scale) {
    if (textLength == 0) return {};

    // all coordinates used and returned here are the unscaled versions; they're scaled later (in render, or shader)
    const float height = (float)font->height();
    const float ascender = (float)font->ascender();
    const float descender = (float)font->descender();
    const float lineSpacing = font->formatting.lineSpacing * height;

    glm::vec2 maxSize = glm::vec2{settings.maxWidth, settings.maxHeight} / scale;

    float cx = 0.0f, mx = 0.0f; // current x, max x
    float y = 0.0f; // y
    int lineLength = 0;
    int wordLength = 0;
    int lineCount = 1;

    CharacterLayoutData layout = {};

    for (int i = 0; i < textLength; i++) {
        char c = text[i];
        bool whitespaceChar = isWhitespace(c); // current character is whitespace (nothing to render, does nothing but shift formatting)
        
        if (!whitespaceChar && (c < font->firstChar || c > font->lastChar)) {
            // turn unsupported characters into hashtags
            c = '#';
        }

        float charPixelsAdvance = (float)font->advance(c);
        
        if (c == '\n') {
            goto newline;
        }
            
        if (cx + charPixelsAdvance + font->size(c).x >= maxSize.x) {
        newline:
            y -= lineSpacing;
            if (y > maxSize.y) {
                // no room for a new line. stop processing text
                break;
            }

            // wrap on whitespace, shift words together
            float wrapOffsetX = 0.0f;
            if (c != '\n' && settings.wrapOnWhitespace && wordLength > 0 && wordLength < lineLength) {
                // index through the last @wordLength characters
                // should be impossible for layout.characters.size to be less than wordLength
                // in the case that a word is so long that it can't even fit on line by itself
                //  (the word length is > maxSize.x), the word would be wrapped onto two lines,
                //  so the word length would be larger than the line length. That's the reason
                //  taking the minimum is necessary here, so we don't shift the line above the
                //  current one, which has already been shifted and is in correct position.
                int segmentLength = MIN(wordLength, lineLength); 
                int startIndex = layout.characters.size() - segmentLength;
                wrapOffsetX = layout.characterOffsets[startIndex].x;
                for (int j = 0; j < segmentLength; j++) {
                    auto* characterOffset = &layout.characterOffsets[startIndex + j];
                    characterOffset->x -= wrapOffsetX;
                    characterOffset->y = y;
                }
            }

            assert(layout.characterOffsets.size() >= lineLength);
            auto line = MutableArrayRef<glm::vec2>(layout.characterOffsets.end() - lineLength, layout.characterOffsets.end());
            float lineSize = cx;

            alignLineHorizontally(line, lineSize, settings.align.horizontal);

            // reset line
            lineLength = 0;
            mx = MIN(MAX(mx, cx), maxSize.x);
            lineCount++;

            if (wrapOffsetX != 0.0f) {
                cx = cx - wrapOffsetX;
            } else {
                cx = 0.0f;
            }
        }

        if (!whitespaceChar) {
            layout.characters.push_back(c);
            layout.characterOffsets.push_back(glm::vec2(cx, y));
            lineLength++;
            wordLength++;
        } else {
            layout.whitespaceCharacters.push_back(c);
            layout.whitespaceCharacterOffsets.push_back(glm::vec2(cx, y));
            wordLength = 0;
        }

        cx += charPixelsAdvance;
    }

    mx = cx > mx ? cx : mx;

    glm::vec2 size = {mx, (lineCount-1) * (lineSpacing) + height};
    glm::vec2 origin = {0, 0};
    glm::vec2 offset = {0, 0};

    float lineMovement = 0.0f;
    switch (settings.align.horizontal) {
    case HoriAlignment::Left:
        break;
    case HoriAlignment::Center:
        lineMovement = cx/2.0f;
        offset.x = -mx/2.0f;
        break;
    case HoriAlignment::Right:
        lineMovement = cx;
        offset.x = -mx;
        break;
    }

    for (int j = 0; j < lineLength; j++) {
        layout.characterOffsets[layout.characters.size() - j - 1].x -= lineMovement;
    }
    
    switch (settings.align.vertical) {
    case VertAlignment::Top:
        break;
    case VertAlignment::Middle:
        offset.y = size.y / 2.0f;
        break;
    case VertAlignment::Bottom:
        offset.y = size.y;
        break;
    default:
        break;
    }

    origin.y = -offset.y + ascender;

    // pos + offset = top left corner

    layout.size = size;
    layout.origin = origin;
    layout.offset = offset;
    return layout;
}

TextRenderer TextRenderer::init(Font* defaultFont) {
    TextRenderer self;
    self.defaultFont = defaultFont;

    self.model = GlGenModel();
    self.model.bindAll();

    const static GlVertexFormat vertexFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)}, // pos
        {2, GL_FLOAT, sizeof(GLfloat)}, // texcoord
        {4, GL_UNSIGNED_BYTE, sizeof(GLubyte), true}, // color, normalized
        {2, GL_FLOAT, sizeof(GLfloat)} // scale
    });

    assert(vertexFormat.totalSize() == sizeof(Vertex));

    vertexFormat.enable();
    
    GlBufferData(
        GL_ARRAY_BUFFER, 
        {maxBatchSize * 4 * sizeof(Vertex), 
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

    /*
    if (font) {
        self.characterTexCoords = Alloc<std::array<TexCoord, 4>>(font->numChars()); 
        self.calculateCharacterTexCoords();
    } else {
        LogError("No font provided for text renderer");
    }
    */

    return self;
}

TextRenderer::RenderResult TextRenderer::render(const char* text, int textLength, glm::vec2 pos, const TextFormattingSettings& formatSettings, RenderingSettings renderSettings, glm::vec2* outCharPositions) {
    if (textLength == 0) return RenderResult::BadRender(pos);

    const Font* font = renderSettings.font ? renderSettings.font : this->defaultFont;
    if (!font || !font->face) {
        return RenderResult::BadRender(pos);
    }

    glm::vec2 scale = renderSettings.scale;
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

        // only make a batch if theres something to render
        //if (layout.characters.size() > 0 || maxSize.x > 0 || maxSize.y > 0) { 
        if (layout.characters.size() > 0) {
            TextRenderLayout batchLayout = {
                .characterOffsets = std::move(layout.characterOffsets),
                .characters = std::move(layout.characters),
                .origin = layout.origin
            };
            auto dimensions = My::Vec<TextRenderBatch::CharacterDimensions>::WithCapacity(batchLayout.characters.size());
            for (int i = 0; i < batchLayout.characters.size(); i++) {
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
                .scales = My::Vec<glm::vec2>::Filled(1, renderSettings.scale)
            };
            
            buffer.push_back(batch);
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

void TextRenderer::renderBatch(const TextRenderBatch* batch, Vertex* verticesOut) const {
    const Font* font = batch->font;
    if (!font) {
        LogError("No font in batch!");
    }
    
    const auto& layout = batch->layout;
    const auto count = layout.characters.size();
    assert(count < maxBatchSize && "Batch too large!");
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

        const Vertex vertices[] = {
            {{x,  y},  {tx, ty2}, color, scale},
            {{x,  y2}, {tx, ty}, color, scale},
            {{x2, y2}, {tx2, ty}, color, scale},
            {{x2, y},  {tx2, ty2}, color, scale}
        };
        
        memcpy(verticesOut + i * 4, vertices, sizeof(vertices));
    }
}

void TextRenderer::flush(glm::mat4 transform) {
   /*
    if (!font) {
        buffer.clear();
        return;
    }

    // check if the precalculated texture coords have been invalidated by a change of scale
    if (font->currentScale != this->texCoordScale) {
        calculateCharacterTexCoords();
    }*/

    glBindVertexArray(model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, model.vbo);

    unsigned int maxCharacters = 0;
    for (auto& batch : buffer) {
        int batchCharacters = batch.layout.characters.size();
        assert(batchCharacters < maxBatchSize && "Character batch too large!");
        maxCharacters = MAX(batchCharacters, maxCharacters);
    }
    const GLuint maxVertices = maxCharacters * 4;
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(Vertex), NULL, GL_STREAM_DRAW);

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
        /*
        const auto* batchFont = batch->font;
        if (batchFont && batchFont != font) {
            setFont(batchFont);
            textShader.setInt("text", batchFont->textureUnit);
        }
        */

        auto* vertexBuffer = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        renderBatch(batch, vertexBuffer);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        glDrawElements(GL_TRIANGLES, 6 * batch->layout.characters.size(), GL_UNSIGNED_SHORT, NULL);
        batch->dimensions.destroy();
        batch->colors.destroy();
        batch->scales.destroy();
    }
    buffer.clear();
    GL::logErrors();
}