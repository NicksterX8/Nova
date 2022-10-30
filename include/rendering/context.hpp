#ifndef RENDERING_CONTEXT_INCLUDED
#define RENDERING_CONTEXT_INCLUDED

#include "../sdl_gl.hpp"
#include "Shader.hpp"
#include "text.hpp"

namespace TextureUnits {
    enum E {
        MyTextureArray=0,
        Text=1
    };
}

using TextureUnit = TextureUnits::E;

struct RenderContext {
    SDL_Window* window = NULL;
    SDL_GLContext glCtx = NULL;
    GLuint textureArray = 0;

    Shader entityShader;
    Shader tilemapShader;
    Shader pointShader;
    Shader colorShader;
    Shader textShader;
    Shader sdfShader;

    Font font;
    Font debugFont;
    TextRenderer textRenderer;

    ModelData chunkModel;

    RenderContext(SDL_Window* window, SDL_GLContext glContext)
    : window(window), glCtx(glContext) {}
};

#endif