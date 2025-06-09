#ifndef SDL_HPP
#define SDL_HPP

#include <SDL3/SDL.h>
#include "constants.hpp"
#include "utils/FileSystem.hpp"
#include "My/String.hpp"
using My::str_add;
#include "sdl_gl.hpp"
#include "utils/Log.hpp"
#include "utils/vectors_and_rects.hpp"

struct WindowContext {
    SDL_Window* window = nullptr;
    SDL_GLContext glContext = NULL;
};

struct SDLContext {
    WindowContext primary;
    WindowContext secondary;
};

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
inline float getPixelScale(SDL_Window* window) {
    // int windowWidth,windowHeight;
    // SDL_GetWindowSize(win, &windowWidth, &windowHeight);
    // int rendererWidth,rendererHeight; // ow and oh for output width and height
    // SDL_GetWindowSizeInPixels(win, &rendererWidth, &rendererHeight);
    
    // // on MacOS when high DPI is enabled, both of these should equal to 2,
    // // Most cases this should just be one.
    // return (float)rendererWidth / (float)windowWidth;
    return SDL_GetWindowPixelDensity(window);
}

inline SDL_FPoint getMousePixelPosition() {
    struct SDL_FPoint mousePos;
    SDL_GetMouseState(&mousePos.x, &mousePos.y);

    mousePos.x *= SDL::pixelScale;
    mousePos.y *= SDL::pixelScale;
    return mousePos;
}

inline Uint32 getMouseButtons() {
    return SDL_GetMouseState(NULL, NULL);
}

}

SDL_Window* createWindow(const char* windowTitle, SDL_Rect windowRect);

SDLContext initSDL(const char* windowTitle, SDL_Rect windowRect);

void quitSDL(SDLContext* ctx);

#endif