#include "rendering/text.hpp"

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
    LogInfo("Unloading font %p", this->face);
    FT_Done_Face(face); face = NULL;
    free(characters);
    glDeleteTextures(1, &atlasTexture);
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