#include "Textures.hpp"
#include "gl.h"
#include "Log.hpp"

int TextureStruct::TileStruct::load(SDL_Renderer* renderer) {
    int code = 0;
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    LOADIMG(grass, "tiles/GrassTile.png");
    LOADIMG(sand, "tiles/sand.png");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    LOADIMG(water, "tiles/water.jpg");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    LOADIMG(error, "tiles/error.png");
    LOADIMG(space, "tiles/space.png");
    LOADIMG(spaceFloor, "tiles/space-floor.png");
    LOADIMG(wall, "tiles/wall.png");

    return 0;
}

void TextureStruct::TileStruct::unload() {
    DELTEX(grass);
    DELTEX(sand);
    DELTEX(water);
    DELTEX(error);
    DELTEX(space);
    DELTEX(spaceFloor);
    DELTEX(wall);
}

int TextureStruct::load(SDL_Renderer* renderer) {
    int code = 0;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    LOADIMG(player, "pixelart/weird-player-guy.png");
    LOADIMG(tree, "tree.png");
    LOADIMG(grenade, "grenade.png")
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    LOADIMG(chest, "chest.png");
    LOADIMG(inserter, "inserter.png");
    code |= Tiles.load(renderer);
    return code;
}

void TextureStruct::unload() {
    DELTEX(player);
    DELTEX(tree);
    DELTEX(grenade);
    DELTEX(chest);
    DELTEX(inserter);
    Tiles.unload();
}

TextureStruct Textures;

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

unsigned int loadGLTexture(const char* filepath) {
    unsigned int texture;
    glGenTextures(1, &texture);  
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
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
        Log.Error("Failed to load texture at \"%s\". SDL error message: %s\n", filepath, SDL_GetError());
        return 0;
    }
    return texture;
}

unsigned int createTextureArray(int width, int height, int depth, SDL_Surface** images) {
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
        if (surface->w > width || surface->h > height) {
            Log.Error("::createTextureArray : Image passed is too large to fit in texture array! image dimensions: %d,%d; texture array dimensions: %d,%d\n", surface->w, surface->h, width, height);
            return 0;
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

int setTextureMetadata() {
    using namespace TextureIDs;
    TextureMetaDataStruct* tx = TextureMetaData;
    
    tx[Player] = {"pixelart/weird-player-guy.png"};
    tx[Inserter] = {"inserter.png"};
    tx[Chest] = {"chest.png"};
    tx[Grenade] = {"grenade.png"};

    tx[Tiles::Grass] = {"tiles/GrassTile.png"};
    tx[Tiles::Sand] = {"tiles/sand.png"};
    tx[Tiles::Water] = {"tiles/water.jpg"};
    tx[Tiles::SpaceFloor] = {"tiles/space-floor.png"};

    // check every texture was set
    int code = 0;
    for (unsigned int i = 0; i < NumTextures; i++) {
        if (tx[i].filename == NULL) {
            Log.Warn("Texture %d was not set!", i);
            code = -1;
        }
    }
    return code;
}

TextureMetaDataStruct TextureMetaData[TextureIDs::NumTextures];
TextureDataStruct TextureData[TextureIDs::NumTextures];

unsigned int makeTextureArray(const char* assetsPath) {
    unsigned int depth = TextureIDs::NumTextures;

    SDL_Surface* images[TextureIDs::NumTextures];
    for (unsigned int i = 0; i < depth; i++) {
        char path[512];
        strcat(strcpy(path, assetsPath), TextureMetaData[i].filename);
        images[i] = IMG_Load(path);
        TextureData[i].width = images[i]->w;
        TextureData[i].height = images[i]->h;
        flipSurface(images[i]);
    }

    for (unsigned int i = 0; i < depth; i++) {
        if (images[i]) {
            if (images[i]->format->format != SDL_PIXELFORMAT_RGBA32) {
                SDL_Surface* newSurface = SDL_ConvertSurfaceFormat(images[i], SDL_PIXELFORMAT_RGBA32, 0);
                if (newSurface) {
                    SDL_FreeSurface(images[i]);
                    images[i] = newSurface;
                } else {
                    printf("Error: Failed to convert surface to proper format!\n");
                }
            }
        } else {
            printf("Failed to load image %d!\n", i);
        }
    }

    unsigned int textureArray = createTextureArray(MY_TEXTURE_ARRAY_WIDTH, MY_TEXTURE_ARRAY_HEIGHT, depth, images);
    // images can be freed after being loaded into texture array
    for (unsigned int i = 0; i < depth; i++) {
        SDL_FreeSurface(images[i]);
    }
    if (!textureArray) {
        Log.Critical("Failed to make texture array!");
    }
    return textureArray;
}