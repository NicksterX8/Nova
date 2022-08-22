#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

#define MY_TEXTURE_ARRAY_WIDTH 128
#define MY_TEXTURE_ARRAY_HEIGHT 128

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

void flipSurface(SDL_Surface* surface);

unsigned int loadGLTexture(const char* filepath);

namespace TextureIDs {
    namespace Tiles {
        enum Tile {
            Grass=0,
            Sand,
            Water,
            SpaceFloor,
            NumTiles
        };
    }
    enum General {
        Inserter = Tiles::NumTiles,
        Chest,
        Player,
        Grenade,
        NumTextures
    };
}

typedef unsigned int TextureID;

struct TextureMetaDataStruct {
    const char* filename = NULL;
};

extern TextureMetaDataStruct TextureMetaData[TextureIDs::NumTextures];

struct TextureDataStruct {
    int width;
    int height;
};
extern TextureDataStruct TextureData[TextureIDs::NumTextures];

unsigned int createTextureArray(int width, int height, int depth, SDL_Surface** images);
int setTextureMetadata();
unsigned int makeTextureArray(const char* assetsPath);


#endif