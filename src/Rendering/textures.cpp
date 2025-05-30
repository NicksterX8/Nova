#include "rendering/textures.hpp"
#include "sdl_gl.hpp"
#include "utils/Log.hpp"
#include "rendering/context.hpp"

void copyTexture(Texture dst, Texture src, glm::ivec2 dstOffset) {
    assert(src.pixelSize == dst.pixelSize); // need same format to copy
    // cut off the extra parts of the src texture
    src.size.x = MIN(dst.size.x, src.size.x);
    src.size.y = MIN(dst.size.y, src.size.y);
    for (int row = 0; row < src.size.y; row++) {
        //emplaceRow(dst, row + dstOffset.y, &src.buffer[row * src.size.x], dstOffset.x, src.size.x);
        int dstByteWidth = dst.size.x * dst.pixelSize;
        int dstRow = row + dstOffset.y;
        int srcByteWidth = src.size.x * src.pixelSize;
        int dstByteColOffset = dstOffset.x * dst.pixelSize;
        int dstByteRowOffset = dstRow * dstByteWidth;
        int srcByteRowOffset = row * srcByteWidth;
        memcpy(&dst.buffer[dstByteRowOffset + dstByteColOffset], &src.buffer[srcByteRowOffset], srcByteWidth);
    }
}

void resizeTexture(Texture* texture, glm::ivec2 newSize) {
    Texture newTexture = newUninitTexture(newSize, texture->pixelSize);
    fillTextureBlack(newTexture);
    copyTexture(newTexture, *texture);
    Free(texture->buffer); 
    *texture = newTexture;
}

void flipSurface(SDL_Surface* surface) {
    SDL_LockSurface(surface);
    
    int pitch = surface->pitch; // row size
    char* temp = new char[pitch]; // intermediate buffer
    char* pixels = (char*)surface->pixels;
    
    for(int i = 0; i < surface->h / 2; ++i) {
        // get pointers to the two rows to swap
        char* row1 = pixels + i * pitch;
        char* row2 = pixels + (surface->h - i - 1) * pitch;
        
        // swap rows
        memcpy(temp, row1, pitch);
        memcpy(row1, row2, pitch);
        memcpy(row2, temp, pitch);
    }
    
    delete[] temp;

    SDL_UnlockSurface(surface);
}

SDL_Surface* loadSurface(const char* filepath, bool flip) {
    SDL_Surface *surface = IMG_Load(filepath);
    if (surface) { 
        if (flip) {
            flipSurface(surface); // May not be necessary
        }

        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (newSurface) {
                SDL_FreeSurface(surface);
                surface = newSurface;
            } else {
                LogError("Couldn't convert surface format while loading \"%s\"!", filepath);
                return nullptr;
            }
        }

        return surface;
    }

    LogError("Failed to load texture with path \"%s\". SDL error message: %s\n", filepath, SDL_GetError());
    return nullptr;
}

GLuint GlLoadTexture(const char* pixels, glm::ivec2 size, GLint minFilter, GLint magFilter) {
    GLuint texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.x, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}

GLuint GlLoadSurface(SDL_Surface* surface, GLint minFilter, GLint magFilter) {
    if (!surface) return 0;
    
    SDL_LockSurface(surface);
    const char* pixels = (char*)surface->pixels;
    glm::ivec2 size = {surface->w, surface->h};
    GLuint tex = GlLoadTexture(pixels, size, minFilter, magFilter);
    SDL_UnlockSurface(surface);
    return tex;
}

SDL_Surface* loadTexture(TextureID id, TextureManager* textures, const char* basePath) {
    const char* filename = textures->metadata[id].filename;
    const char* identifier = textures->metadata[id].identifier;
    if (!filename) {
        LogWarn("No file for texture %d!", id);
        return nullptr;
    }
    auto path = My::str_add(basePath, filename); // de allocated at end of scope
    SDL_Surface* image = IMG_Load(path);
    if (!image) {
        LogError("Failed to load texture \"%s\" with path: \"%s\". Error: %s", identifier, (char*)path, SDL_GetError());
        textures->data[id] = TextureData{{0, 0}};
        return nullptr;
    }
    textures->data[id] = TextureData{{image->w, image->h}};
    if (image->format->format != StandardPixelFormat) {
        SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(image, StandardPixelFormat, 0);
        SDL_FreeSurface(image);
        if (newSurface) {
            image = newSurface;
        } else {
            LogError("Failed to load texture \"%s\": couldn't convert surface to proper format! Error: %s\n", identifier, SDL_GetError());
            return nullptr;
        }
    }
    flipSurface(image);
    return image;
}

GLuint GlLoadTextureArray(glm::ivec2 size, ArrayRef<SDL_Surface*> images, GLenum minFilter, GLenum magFilter) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0,                  // level
        GL_RGBA8,           // internal format
        size.x, size.y, images.size(), // width,height,depth
        0,                  // border?
        GL_RGBA,            // format
        GL_UNSIGNED_BYTE,   // type
        nullptr             // pointer to data, left empty to be loaded with images
    );
    // load images one by one, each on a different layer (depth)
    for (unsigned int i = 0; i < images.size(); i++) {
        SDL_Surface* surface = images[i];
        if (!surface) continue;
        if (surface->w > size.x || surface->h > size.y) {
            LogError("createTextureArray : Image passed is too large to fit in texture array! image dimensions: %d,%d;"
                "texture array dimensions: %d,%d\n", surface->w, surface->h, size.x, size.y);
            continue;
        }
        SDL_LockSurface(surface);
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, // target is texture array
            0,
            0, 0, i, // all images start at bottom left of texture array
            surface->w, surface->h, 1, // image is only 1 thick in depth
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            surface->pixels
        );
        SDL_UnlockSurface(surface);
    }

    return texture;
}

TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, TextureType typesIncluded, const char* assetsPath, TextureUnit textureUnit) {
    llvm::SmallVector<SDL_Surface*> images;
    llvm::SmallVector<TextureID> ids;
    TextureMetaData* metadata = textures->metadata.data;

    for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
        if ((metadata[id].type & typesIncluded) || true) {
            if (textures->animations.lookup(id)) continue; // dont include animations
            auto image = loadTexture(id, textures, assetsPath);
            if (image) {
                images.push_back(image);
                ids.push_back(id);
            }
        }
    }

    TextureArray texArray = TextureArray(size, images.size(), 0, textureUnit);
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    GLuint texture = GlLoadTextureArray(size, images, GL_NEAREST, GL_NEAREST);
    texArray.texture = texture;
    
    if (!texArray.texture) {
        LogCritical("Failed to make texture array!");
    }

    // images can be freed after being loaded into texture array
    for (auto image : images) {
        SDL_FreeSurface(image);
    }

    for (int i = 0; i < images.size(); i++) {
        texArray.textureDepths.insert(ids[i], i);
    }

    return texArray;
}

int updateTextureArray(TextureArray* textureArray, TextureManager* textures, TextureID id, SDL_Surface* surface) {
    if (!textureArray || !surface) return -1;
    if (id < TextureIDs::First || id > TextureIDs::Last) {
        LogError("Tried to set invalid texture id: %d, texture array depth: %d", id, textureArray->depth);
        return -1;
    }

    int depth = getTextureArrayDepth(textureArray, id);
    if (depth == -1) {
        LogError("Texture %d not found in texture array!", id);
        return -1;
    }

    glActiveTexture(GL_TEXTURE0 + textureArray->unit);

    SDL_LockSurface(surface);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, // target is texture array
        0, // mipmap level (i think) no mipmaps
        0, 0, depth, // all images start at bottom left of texture array
        surface->w, surface->h, 1, // image is only 1 thick in depth
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        surface->pixels
    );
    SDL_UnlockSurface(surface);

    textures->data[id] = {{surface->w, surface->h}};

    return 0;
}

GlSizedTexture GlLoadTextureAtlas(ArrayRef<SDL_Surface*> images, GLint minFilter, GLint magFilter, MutableArrayRef<glm::ivec2> texCoordsOut) {
    auto* textures = Alloc<Texture>(images.size());
    for (int i = 0; i < images.size(); i++) {
        auto* image = images[i];
        SDL_LockSurface(image);
        textures[i] = newTexture({images[i]->w, images[i]->h}, StandardPixelFormatBytes, (unsigned char*)image->pixels);
        SDL_UnlockSurface(image);
    }
    assert(texCoordsOut.size() >= images.size());
    Texture packedTexture = packTextures(images.size(), textures, StandardPixelFormatBytes, texCoordsOut.data());

    for (int i = 0; i < images.size(); i++) {
        freeTexture(textures[i]);
    }
    Free(textures);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        packedTexture.size.x, packedTexture.size.y,
        0, // no border i think
        GL_RGBA, GL_UNSIGNED_BYTE,
        packedTexture.buffer
    );

    freeTexture(packedTexture);

    return {texture, packedTexture.size};
}

TextureAtlas makeTextureAtlas(TextureManager* textures, TextureType typesIncluded, const char* assetsPath, GLint minFilter, GLint magFilter, TextureUnit textureUnit) {
    llvm::SmallVector<SDL_Surface*> images;
    llvm::SmallVector<TextureID> ids;
    TextureMetaData* metadata = textures->metadata.data;

    for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
        if (textures->metadata[id].type & typesIncluded) {
            auto image = loadTexture(id, textures, assetsPath);
            if (image) {
                images.push_back(image);
                ids.push_back(id);
            }
        }
    }

    auto* textureOrigins = Alloc<glm::ivec2>(images.size());

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    auto textureAndSize = GlLoadTextureAtlas(images, minFilter, magFilter, {textureOrigins, images.size()} /* to be filled in */);
    if (!textureAndSize.texture) {
        LogCritical("Failed to make texture array!");
    }
    TextureAtlas atlas = TextureAtlas(textureAndSize.size, textureAndSize.texture, textureUnit);

    TextureAtlas::Space* textureSpaces = atlas.textureSpaces.insertList(ids);
    for (int i = 0; i < ids.size(); i++) {
        auto origin = TextureAtlas::TexCoord(textureOrigins[i]);
        auto size = TextureAtlas::TexCoord{images[i]->w, images[i]->h};
        textureSpaces[i] = {
            origin,
            origin + size
        };
    }

    Free(textureOrigins);

    // images can be freed after being loaded into texture array
    for (auto image : images) {
        SDL_FreeSurface(image);
    }

    return atlas;
}

static void addAnimation(const Animation& animation, TextureManager* textureManager) {
    textureManager->addAnimation(&animation);
}

int setTextureMetadata(TextureManager* textureManager) {
    if (!textureManager) return -1;

    using namespace TextureIDs;
    using namespace TextureIDs::Tiles;

    #define TEX(id, type, filename) textureManager->add(TextureIDs::id, TOSTRING(id), TextureTypes::type, filename)
    
    TEX(Null, Other, NULL);
    TEX(Player, World, "player.png");
    TEX(PlayerShadow, World, "player-shadow.png");
    TEX(Inserter, World, "inserter.png");
    TEX(Chest, World, "chest.png");
    TEX(Grenade, World, "grenade.png");
    TEX(TreeBottom, World, "tree-bottom.png");
    TEX(TreeTop, World, "tree-top.png");

    #define TILE(id, filename) TEX(Tiles::id, World, "tiles/" filename)
    #define GUI(id, filename) TEX(GUI::id, Gui, "gui/" filename)

    TILE(Grass, "grass.png");
    TILE(Sand, "sand.png");
    TEX(Tiles::Water, World, "tiles/grass.png");
    TILE(SpaceFloor, "space-floor.png");
    TILE(Wall, "wall.png");
    TILE(GreyFloor, "grey-floor.png");
    TILE(GreyFloorOverhang, "grey-floor-overhang.png");

    #define ANIMATION(id, type, filename, frameCount, frameSize, duration) {Animation animation = {TextureIDs::id, frameSize, frameCount, duration}; textureManager->add(TextureIDs::id, TOSTRING(id), TextureTypes::type, filename, &animation);}

    TEX(Buildings::TransportBelt, World, "buildings/belt.png");
    TEX(Buildings::TransportBeltAnimation, World, "buildings/belt-animation.png");
    Animation beltAnimation = {
        Buildings::TransportBeltAnimation,
        .frameSize = {32, 32},
        .frameCount = 16,
        .updatesPerFrame = 2
    };
    addAnimation(beltAnimation, textureManager);

    //GUI(SquareButton, "square-button.png");

    // check every texture was set
    int code = 0;
    // start at 1 to skip null texture
    for (unsigned int i = 1; i < TextureIDs::NumTextures; i++) {
        auto& metadata = textureManager->metadata[i];
        if (metadata.filename == NULL || metadata.identifier == NULL) {
            LogWarn("Texture %d was not set!", i);
            code = -1;
        }
    }
    return code;
}