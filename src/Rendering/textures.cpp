#include "rendering/textures.hpp"
#include "sdl_gl.hpp"
#include "utils/Log.hpp"
#include "rendering/context.hpp"

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

unsigned int loadGLTexture(Texture rawTexture) {
    unsigned int texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, rawTexture.size.x, rawTexture.size.x, 0, GL_RGBA, GL_UNSIGNED_BYTE, rawTexture.buffer);

    glGenerateMipmap(GL_TEXTURE_2D);
    return texture;
}

unsigned int loadGLTexture(const char* filepath) {
    unsigned int texture;
    glGenTextures(1, &texture);  
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    SDL_Surface *surface = IMG_Load(filepath);
    if (surface) { 
        flipSurface(surface);

        if (surface->format->format != SDL_PIXELFORMAT_RGBA32) {
            SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
            if (newSurface) {
                SDL_FreeSurface(surface);
                surface = newSurface;
            } else {
                printf("Couldn't convert surface format while loading \"%s\"!", filepath);
                return 0;
            }
        }
        
        SDL_LockSurface(surface);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
        SDL_UnlockSurface(surface);
        glGenerateMipmap(GL_TEXTURE_2D);
        SDL_FreeSurface(surface);
    } else {
        LogError("Failed to load texture at \"%s\". SDL error message: %s\n", filepath, SDL_GetError());
        return 0;
    }
    return texture;
}

Texture loadTexture(const char* filepath, bool flip) {
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
                printf("Couldn't convert surface format while loading \"%s\"!", filepath);
                return NullTexture;
            }
        }

        SDL_LockSurface(surface);
        Texture texture = newUninitTexture({surface->w, surface->h}, RGBA32PixelSize);
        memcpy(texture.buffer, surface->pixels, surface->w * surface->h * RGBA32PixelSize);
        SDL_UnlockSurface(surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    LogError("Failed to load texture at \"%s\". SDL error message: %s\n", filepath, SDL_GetError());
    return NullTexture;
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

GLuint loadTextureArray(glm::ivec2 size, ArrayRef<SDL_Surface*> images, ArrayRef<TextureID> ids, My::DenseSparseSet<TextureID, int, TextureIDs::NumTextures>& textureDepths) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

        textureDepths.insert(ids[i], (int)i);
    }

    return texture;
}

TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, TextureType typesIncluded, const char* assetsPath) {
    llvm::SmallVector<SDL_Surface*> images;
    llvm::SmallVector<TextureID> ids;
    TextureMetaData* metadata = textures->metadata.data;

    for (TextureID id = TextureIDs::First; id <= TextureIDs::Last; id++) {
        if ((metadata[id].type & typesIncluded) || true) {
            auto image = loadTexture(id, textures, assetsPath);
            if (image) {
                images.push_back(image);
                ids.push_back(id);
            }
        }
    }

    TextureArray texArray = TextureArray(size, images.size(), 0);
    GLuint texture = loadTextureArray(size, images, ids, texArray.textureDepths);
    texArray.texture = texture;
    
    if (!texArray.texture) {
        LogCritical("Failed to make texture array!");
    }

    // images can be freed after being loaded into texture array
    for (auto image : images) {
        SDL_FreeSurface(image);
    }

    /*
    for (int i = 0; i < images.size(); i++) {
        texArray.textureDepths.insert(ids[i], i);
    }*/

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

    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray->texture);

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

GlSizedTexture loadTextureAtlas(ArrayRef<SDL_Surface*> images, MutableArrayRef<glm::ivec2> texCoordsOut) {
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

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

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

TextureAtlas makeTextureAtlas(TextureManager* textures, TextureType typesIncluded, const char* assetsPath) {
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

    auto textureAndSize = loadTextureAtlas(images, {textureOrigins, images.size()} /* to be filled in */);
    if (!textureAndSize.texture) {
        LogCritical("Failed to make texture array!");
    }
    TextureAtlas atlas = TextureAtlas(textureAndSize.size, textureAndSize.texture);
    
    // images can be freed after being loaded into texture array
    for (auto image : images) {
        SDL_FreeSurface(image);
    }

    TextureAtlas::Space* textureSpaces = atlas.textureSpaces.insertList(ids);
    for (int i = 0; i < ids.size(); i++) {
        auto origin = TextureAtlas::TexCoord(textureOrigins[i]);
        auto size = TextureAtlas::TexCoord{images[i]->w, images[i]->h};
        textureSpaces[i] = {
            origin,
            origin + size
        };
    }

    return atlas;
}

int setTextureMetadata(TextureManager* textureManager) {
    if (!textureManager) return -1;

    #define TEX(id, type, filename) textureManager->add(TextureIDs::id, TOSTRING(id), TextureTypes::type, filename)
    
    TEX(Null, Other, NULL);
    TEX(Player, World, "player.png");
    TEX(Inserter, World, "inserter.png");
    TEX(Chest, World, "chest.png");
    TEX(Grenade, World, "grenade.png");
    TEX(Tree, World, "tree.png");

    #define TILE(id, filename) TEX(Tiles::id, World, "tiles/" filename)
    #define GUI(id, filename) TEX(GUI::id, Gui, "gui/" filename)

    TILE(Grass, "grass.png");
    TILE(Sand, "sand.png");
    TILE(Water, "../tree.png");
    TILE(SpaceFloor, "space-floor.png");
    TILE(Wall, "wall.png");

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