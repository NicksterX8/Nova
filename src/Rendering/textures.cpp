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
        Texture texture = createUninitTexture({surface->w, surface->h}, RGBA32PixelSize);
        memcpy(texture.buffer, surface->pixels, surface->w * surface->h * RGBA32PixelSize);
        SDL_UnlockSurface(surface);
        SDL_FreeSurface(surface);
        return texture;
    }

    LogError("Failed to load texture at \"%s\". SDL error message: %s\n", filepath, SDL_GetError());
    return NullTexture;
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

unsigned int loadTextureArray(int width, int height, int depth, SDL_Surface** images) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
        0,                  // level
        GL_RGBA8,           // internal format
        width,height,depth, // width,height,depth
        0,                  // border?
        GL_RGBA,            // format
        GL_UNSIGNED_BYTE,   // type
        0                   // pointer to data, left empty to be loaded with images
    );
    // load images one by one, each on a different layer (depth)
    for (unsigned int i = 0; i < depth; i++) {
        SDL_Surface* surface = images[i];
        if (!surface) continue;
        if (surface->w > width || surface->h > height) {
            LogError("createTextureArray : Image passed is too large to fit in texture array! image dimensions: %d,%d; texture array dimensions: %d,%d\n", surface->w, surface->h, width, height);
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

int setTextureMetadata(TextureManager* textureManager) {
    if (!textureManager) return -1;

    #define TEX(id, filename) textureManager->push(TextureIDs::id, TOSTRING(id), filename)
    
    TEX(Null, NULL);
    TEX(Player, "player.png");
    TEX(Inserter, "inserter.png");
    TEX(Chest, "chest.png");
    TEX(Grenade, "grenade.png");
    TEX(Tree, "tree.png");

    #define TILE(id, filename) TEX(Tiles::id, "tiles/" filename)

    TILE(Grass, "grass.png");
    TILE(Sand, "sand.png");
    TILE(Water, "water.png");
    TILE(SpaceFloor, "space-floor.png");
    TILE(Wall, "wall.png");

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

SDL_Surface** loadTextures(TextureManager* textures, const char* assetsPath) {
    SDL_Surface** images = Alloc<SDL_Surface*>(TextureIDs::NumTextures);
    TextureMetaData* metadata = textures->metadata.data;

    char path[512];
    strncpy(path, assetsPath, 512);
    int basePathLen = strlen(assetsPath);

    for (int i = 0; i < TextureIDs::NumTextures; i++) {
        TextureID textureID = i + TextureIDs::First;
        const char* filename = metadata[textureID].filename;
        if (!filename) {
            LogError("Texture metadata %d is NULL!", i);
            images[i] = NULL;
            continue;
        }
        memcpy(path + basePathLen, filename, strlen(filename) + 1);
        images[i] = IMG_Load(path);
        if (!images[i]) {
            LogError("Failed to load texture \"%s\" with path: \"%s\"", metadata[textureID].identifier, path);
            textures->data[textureID] = TextureData{{0, 0}};
            continue;
        }
        textures->data[textureID] = TextureData{{images[i]->w, images[i]->h}};
        flipSurface(images[i]);
    }

    for (unsigned int i = 0; i < TextureIDs::NumTextures; i++) {
        if (images[i]) {
            if (images[i]->format->format != StandardPixelFormat) {
                SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(images[i], StandardPixelFormat, 0);
                if (newSurface) {
                    SDL_FreeSurface(images[i]);
                    images[i] = newSurface;
                } else {
                    LogError("Failed to convert surface to proper format!\n");
                }
            }
        }
    }

    return images;
}

TextureArray makeTextureArray(glm::ivec2 size, TextureManager* textures, const char* assetsPath) {
    TextureArray texArray;
    texArray.depth = TextureIDs::NumTextures;
    texArray.size = size;

    SDL_Surface** images = loadTextures(textures, assetsPath);

    texArray.texture = loadTextureArray(texArray.size.x, texArray.size.y, texArray.depth, images);

    // images can be freed after being loaded into texture array
    for (int i = 0; i < texArray.depth; i++) {
        if (images[i]) {
            SDL_FreeSurface(images[i]);
        }
    }
    if (!texArray.texture) {
        LogCritical("Failed to make texture array!");
    }
    return texArray;
}

int updateTextureArray(TextureArray* textureArray, int depth, SDL_Surface* surface) {
    if (!textureArray || !surface) return -1;
    if (depth < 0 || depth >= textureArray->depth) {
        LogError("Tried to set out of range texture depth. Depth: %d, texture array depth: %d", depth, textureArray->depth);
        return -1;
    }

    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray->texture);

    SDL_LockSurface(surface);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, // target is texture array
        0,
        0, 0, depth, // all images start at bottom left of texture array
        surface->w, surface->h, 1, // image is only 1 thick in depth
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        surface->pixels
    );
    SDL_UnlockSurface(surface);
    return 0;
}