#include "Textures.hpp"
#include "gl.h"
#include "Log.hpp"

FC_Font *FreeSans;

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