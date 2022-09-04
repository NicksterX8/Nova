#ifndef TEXTURES_INCLUDED
#define TEXTURES_INCLUDED

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>

#define MY_TEXTURE_ARRAY_WIDTH 128
#define MY_TEXTURE_ARRAY_HEIGHT 128
#define MY_TEXTURE_ARRAY_UNIT 0

void flipSurface(SDL_Surface* surface);

unsigned int loadGLTexture(const char* filepath);

typedef Uint32 TextureID;

namespace TextureIDs {
    enum Extra : TextureID {
        Null=0,
    };
    namespace Tiles {
        enum Tile : TextureID {
            Grass = Null+1,
            Sand,
            Water,
            SpaceFloor,
            Wall,
            lastTile
        };
    }
    enum General : TextureID {
        Inserter = Tiles::lastTile,
        Chest,
        Grenade,
        Player,
        Tree,
        Last
    };

    constexpr TextureID First = Null+1;
    constexpr TextureID NumTextures = Last-1;
}

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