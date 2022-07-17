#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "SDL_FontCache/SDL_FontCache.h"
#include <string>

extern FC_Font *FreeSans;

#define ASSETS_PATH "assets/"
#define LOADIMG(var, filename) {\
    var = IMG_LoadTexture(renderer, ASSETS_PATH filename);\
    code |= (!var);}
#define DELTEX(var) {SDL_DestroyTexture(var);}
struct TextureStruct {
    SDL_Texture* player;
    SDL_Texture* tree;
    SDL_Texture* grenade;
    SDL_Texture* chest;
    SDL_Texture* inserter;
    struct TileStruct {
        SDL_Texture* grass;
        SDL_Texture* sand;
        SDL_Texture* water;
        SDL_Texture* error;
        SDL_Texture* space;
        SDL_Texture* spaceFloor;
        SDL_Texture* wall;

        int load(SDL_Renderer* renderer);

        void unload();

    } Tiles;

    inline std::string getNameFromTexture(SDL_Texture* texture) {
        if (texture == player) {
            return "player";
        }
        return "";
    }

    inline SDL_Texture* getTextureFromName(const char* name) {
        if (strcmp(name, "player") == 0) {
            return player;
        }
        return NULL;
    }

    int load(SDL_Renderer* renderer);

    void unload();
};

extern TextureStruct Textures;

#endif