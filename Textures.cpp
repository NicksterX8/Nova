#include "Textures.hpp"

FC_Font *FreeSans;

int TextureStruct::TileStruct::load(SDL_Renderer* renderer) {
    int code = 0;
    
    LOADIMG(grass, "tiles/GrassTile.png");
    LOADIMG(sand, "tiles/sand.png");
    LOADIMG(water, "tiles/water.jpg");
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
    LOADIMG(player, "pixelart/weird-player-guy.png");
    LOADIMG(tree, "tree.png");
    LOADIMG(grenade, "grenade.png")
    code |= Tiles.load(renderer);
    return code;
}

void TextureStruct::unload() {
    DELTEX(player);
    DELTEX(tree);
    DELTEX(grenade);
    Tiles.unload();
}

TextureStruct Textures;