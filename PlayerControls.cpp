#include "PlayerControls.hpp"

MouseState getMouseState() {
    int mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDLPixelScale;
    mouseY *= SDLPixelScale;
    return {
        mouseX,
        mouseY,
        buttons
    };
}