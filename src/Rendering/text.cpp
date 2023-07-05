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

struct List {
    void* data;
    size_t stride;
};

template<typename T>
struct ListT {
    T* data;
    size_t stride;
    
};


Font::Font(float _tabWidthScale, float _lineHeightScale, const char* fontfile, FT_UInt baseHeight, float scale, bool useSDFs, char _firstChar, char _lastChar, GLuint textureUnit) {
    this->baseHeight = baseHeight;

    FT_UInt _height = baseHeight * scale;
    this->scale = scale;

    tabWidthScale = _tabWidthScale;
    lineHeightScale = _lineHeightScale;

    if (_lastChar > 127 || _firstChar > 127) {
        LogError("Can't load font characters higher than 127!");
        if (_lastChar > 127) _lastChar = 127;
        if (_firstChar > 127) _firstChar = 127;
    }
    
    firstChar = _firstChar;
    lastChar = _lastChar;

    usingSDFs = useSDFs;

    FT_Error err = 0;
    err = FT_New_Face(freetype, fontfile, 0, &this->face);
    if (err || !this->face) {
        LogError("Failed to load font face. Filepath: %s. Error: %s", fontfile, FT_Error_String(err));
        this->firstChar = 0;
        this->lastChar = -1;
        return;
    }

    char numChars = lastChar - firstChar;
    if (numChars > 0) {
        size_t nChars = (size_t)numChars;
        this->characters.advances  = Alloc<unsigned short>(nChars);
        this->characters.bearings  = Alloc<glm::ivec2>(nChars);
        this->characters.positions = Alloc<glm::ivec2>(nChars);
        this->characters.sizes     = Alloc<glm::ivec2>(nChars);
        load(_height, textureUnit, useSDFs);
    }
}

void Font::load(FT_UInt height, GLuint textureUnit, bool useSDFs) {
    usingSDFs = useSDFs;

    FT_Error err = FT_Set_Pixel_Sizes(face, 0, height);
    //assert(face->height >> 6 == height);
    if (err) {
        LogError("Failed to set font face pixel size. FT_Error: %s", FT_Error_String(err));
        return;
    }
    char numChars = lastChar - firstChar;
    if (numChars <= 0) return;
    size_t nChars = (size_t)numChars;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    if (this->face) {
        FT_GlyphSlot slot = face->glyph;

        FT_Error error;
        
        Texture* characterTextures = Alloc<Texture>(nChars);
    
        for (size_t i = 0; i < nChars; i++) {
            char c = firstChar + (char)i;
            // load character glyph 
            if ((error = FT_Load_Char(face, c, FT_LOAD_RENDER))) {
                LogError("Failed to load glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                continue;
            }

            if (useSDFs) {
                if (c != ' ') {
                    if ((error = FT_Render_Glyph(slot, FT_RENDER_MODE_SDF))) {
                        LogError("Failed to load sdf glyph charcter \'%c\'. Error: %s", c, FT_Error_String(error));
                        continue;
                    }
                }
            }

            characters.advances[i] = slot->advance.x;
            characters.bearings[i] = glm::ivec2{slot->bitmap_left, slot->bitmap_top};
            characters.sizes[i]    = glm::ivec2{slot->bitmap.width, slot->bitmap.rows};
            characterTextures[i] = newUninitTexture(characters.sizes[i], 1);
            memcpy(characterTextures[i].buffer, slot->bitmap.buffer, slot->bitmap.rows * slot->bitmap.width);
            // maybe switch thsi back
            //characterTextures[i]   = {.buffer = slot->bitmap.buffer, .size = characters.sizes[i], .pixelSize = 1}; // Character glyphs use 1 byte pixels, black and white
        }

        GL::logErrors();
        auto packedTexture = packTextures(
            numChars, characterTextures,
            1, // 1 byte pixels
            characters.positions
        );

        this->atlasSize = packedTexture.size;
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        this->atlasTexture = loadFontAtlasTexture(packedTexture);
        Free(characterTextures);
    }
    GL::logErrors();
    
    if (!this->atlasTexture) {
        LogError("Font atlas texture failed to generate!");
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // re enable default byte-alignment restriction
}

 void Font::unload() {
    LogInfo("Unloading font %p", this->face);
    FT_Done_Face(face); face = NULL;
    Free(characters.advances);
    Free(characters.bearings);
    Free(characters.positions);
    Free(characters.sizes);
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