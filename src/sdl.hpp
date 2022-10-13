#ifndef SDL_HPP
#define SDL_HPP

#include <SDL2/SDL.h>
#include "constants.hpp"
#include "utils/FileSystem.hpp"
#include "My/String.hpp"
using My::str_add;
#include "sdl_gl.hpp"
#include "utils/Log.hpp"
#include "utils/Vectors.hpp"

// sdl image header broken for some reason so we just make our own for what we use
/*
extern "C" {
    SDL_Surface* IMG_Load(const char*);
    int IMG_Init(int flags);
    enum ImgInitFlags {
        IMG_INIT_JPG=1,
        IMG_INIT_PNG=2
    };
}
*/

#define WINDOW_HIGH_DPI 1

typedef struct SDLContext {
    SDL_Window *win;
    SDL_GLContext gl;
    float scale;
} SDLContext;

namespace SDL {

extern float pixelScale;

/*
* Get the ratio or scale of window size to renderer output size. 
* Useful to find how accurate the window size is to output screen size. 
* Only necessary to call this once when the window and renderer initialized 
* because this shouldn't change during program lifetime. 
* If the scale for some reason is not 1:1 for the width and height values, returns the scale for the width.
* @return the scale of the window coordinates to the renderer 
*/
inline Vec2 getPixelScale(SDL_Window *win) {
    int windowWidth,windowHeight;
    SDL_GetWindowSize(win, &windowWidth, &windowHeight);
    int rendererWidth,rendererHeight; // ow and oh for output width and height
    SDL_GL_GetDrawableSize(win, &rendererWidth, &rendererHeight);
    
    // on MacOS when high DPI is enabled, both of these should equal to 2,
    // Most cases this should just be one.
    return {
        (float)rendererWidth / windowWidth,
        (float)rendererHeight / windowHeight
    };
}

inline SDL_Point getMousePixelPosition() {
    struct SDL_Point mousePos;
    SDL_GetMouseState(&mousePos.x, &mousePos.y);

    mousePos.x *= SDL::pixelScale;
    mousePos.y *= SDL::pixelScale;
    return mousePos;
}

inline Uint32 getMouseButtons() {
    return SDL_GetMouseState(NULL, NULL);
}

}

SDLContext initSDL();

void quitSDL(SDLContext* ctx);

#endif