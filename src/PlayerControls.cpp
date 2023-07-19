#include "PlayerControls.hpp"
#include "Game.hpp"

MouseState getMouseState() {
    int mouseX,mouseY;
    Uint32 buttons = SDL_GetMouseState(&mouseX,&mouseY);
    mouseX *= SDL::pixelScale;
    mouseY *= SDL::pixelScale;
    return {
        mouseX,
        mouseY,
        buttons
    };
}