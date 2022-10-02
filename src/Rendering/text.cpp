#include "text.hpp"

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

void Font::load(const char* fontfile, FT_UInt height, bool useSDFs, char firstChar, char lastChar) {
    if (lastChar > 127 || firstChar > 127) {
        LogError("Can't load font characters higher than 127!");
        if (lastChar > 127) lastChar = 127;
        if (firstChar > 127) firstChar = 127;
    }
    this->firstChar = firstChar;
    this->lastChar = lastChar;

    this->face = newFontFace(fontfile, height);

    this->characters = new Character[lastChar - firstChar];

    TexturePacker packer({256, 256});

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    FT_GlyphSlot slot = face->glyph;

    FT_Error error;
    
    for (unsigned char c = firstChar; c <= lastChar; c++) {
        // load character glyph 
        if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER))) {
            LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
            continue;
        }

        // TODO: move this out of loop for better performance
        if (useSDFs) {
            if (c != ' ') {
                if ((error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF))) {
                    LogError("Failed to load sdf glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                    continue;
                }
            }
        }

        glm::ivec2 texPos = packer.packTexture(slot->bitmap.buffer, glm::ivec2(slot->bitmap.width, slot->bitmap.rows));

        // now store character for later use3
        characters[c-firstChar] = Font::Character{
            texPos,
            glm::ivec2(slot->bitmap.width, slot->bitmap.rows),
            glm::ivec2(slot->bitmap_left, slot->bitmap_top),
            (unsigned int)slot->advance.x
        };
    }

    this->atlasSize = packer.textureSize;
    this->atlasTexture = generateFontAtlasTexture(packer);
    if (!atlasTexture) {
        LogError("Font atlas texture failed to generate!");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

 void Font::unload() {
    FT_Done_Face(face);
    delete[] characters;
    glDeleteTextures(1, &atlasTexture);
}

static vao_vbo_t renderTextInitBuffers() {
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_UNSIGNED_INT, GL_FALSE, 1 * sizeof(GLuint), (void*)(2 * sizeof(GLfloat)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return {VAO, VBO};
}

void renderTextGeneric(Shader& shader, const Font& font, const char* text, const float x, const float y, const float scale, glm::vec3 color, float lineSpaceHeight, float tabSpacesWidth) {
    float cx = x;
    float cy = y;

    // activate corresponding render state	
    shader.use();
    shader.setVec3("textColor", color);
    static auto buffers = renderTextInitBuffers();
    glBindVertexArray(buffers.vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
    GLvoid* vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

    glBindTexture(GL_TEXTURE_2D, font.atlasTexture);

    int textLen = 0;
    for (const char* c = text; *c != '\0'; c++) {
        if (*c == '\n') {
            cy -= lineSpaceHeight * (font.face->height >> 6) * scale;
            cx = x;
            continue;
        }
        else if (*c == '\t') {
            // tab
            cx += tabSpacesWidth * (font.characters[' ' - font.firstChar].advance >> 6) * scale;
            continue;
        }

        if (*c < font.firstChar || *c > font.lastChar) {
            // skip unsupported characters
            continue;
        }

        Font::Character ch = font.characters[*c-font.firstChar];
        if (*c == ' ') {
            cx += (ch.advance >> 6) * scale; 
            continue;
        }

        float x = cx + ch.bearing.x * scale;
        float y = cy - (ch.size.y - ch.bearing.y) * scale;
        float x2 = x + ch.size.x * scale;
        float y2 = y + ch.size.y * scale;

        float tx = ch.texPos.x / (float)font.atlasSize.x;
        float ty = ch.texPos.y / (float)font.atlasSize.y;
        float tx2 = (ch.texPos.x + ch.size.x) / (float)font.atlasSize.x;
        float ty2 = (ch.texPos.y + ch.size.y) / (float)font.atlasSize.y;
        // update VBO for each character
        // rendering glyph texture over quad
        struct Vertex {
            GLfloat pos[2];
            GLuint texCoords;
        };
        float vertices[6][4] = {
            { x,  y,   tx,  ty2 },            
            { x,  y2,  tx,  ty  },
            { x2, y2,  tx2, ty  },

            { x,  y,   tx,  ty2 },
            { x2, y,   tx2, ty2 },
            { x2, y2,  tx2, ty  }           
        };
        
        // add to vertex buffer
        memcpy((char*)vertexBuffer + textLen * sizeof(vertices), vertices, sizeof(vertices));
        
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        cx += (ch.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
        textLen++;
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
    // render quads
    glDrawArrays(GL_TRIANGLES, 0, 6 * textLen);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}