#include "rendering/text/Font.hpp"
#include "rendering/TexturePacker.hpp"

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

Font* newFont(const char* fontfile, FT_UInt baseHeight, bool useSDFs, float scale, Font::FormattingSettings formatting, TextureUnit textureUnit, Shader shader) {
    Font* f = new Font();
    
    f->baseHeight = baseHeight;

    FT_UInt _height = baseHeight * scale;
    f->currentScale = scale;

    f->formatting = formatting;
    f->textureUnit = textureUnit;

    f->usingSDFs = useSDFs;

    FT_Error err = 0;
    err = FT_New_Face(freetype, fontfile, 0, &f->face);
    if (err || !f->face) {
        LogError("Failed to load font face. Filepath: %s. Error: %s", fontfile, FT_Error_String(err));
        delete f;
        return nullptr;
    }

    f->characters = Alloc<Font::AtlasCharacterData>();
    f->load(_height, shader, useSDFs);

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

    constexpr int nChars = (int)ASCII_LAST_STANDARD_CHAR - (int)ASCII_FIRST_STANDARD_CHAR + 1;

    constexpr int pixelSize = 1; // Character glyphs use 1 byte pixels, black and white
    glPixelStorei(GL_UNPACK_ALIGNMENT, pixelSize); // disable byte-alignment restriction

    memset(characters, 0, sizeof(decltype(*characters)));

    if (this->face) {
        if (hasKerning()) LogInfo("kerning possible on %s", face->family_name);

        FT_GlyphSlot slot = face->glyph;

        auto textureAtlas = makeTexturePackingAtlas(pixelSize, nChars);
        for (size_t i = 0; i < nChars; i++) {
            FT_Error error;
            char c = ASCII_FIRST_STANDARD_CHAR + (char)i;
            // load character glyph 
            if ((error = FT_Load_Char(face, c, FT_LOAD_DEFAULT))) {
                LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                continue;
            }

            if (c != ' ') {
                auto mode = useSDFs ? FT_RENDER_MODE_SDF : FT_RENDER_MODE_NORMAL;
                bool usingBSDFs = usingSDFs && 0;
                if (usingBSDFs) {
                    if ((error = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL))) {
                        LogError("Failed to load sdf glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                    }
                }
                if ((error = FT_Render_Glyph(slot, mode))) {
                    LogError("Failed to load sdf glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                    continue;
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