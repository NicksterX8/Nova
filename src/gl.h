#ifndef GL_H_INCLUDED
#define GL_H_INCLUDED

#define GL_SILENCE_DEPRECATION 1 // openGL is deprecated on macos and makes tons of warnings unless silenced with this.
#include <OpenGL/gl3.h> // this needs to be included before sdl opengl is so it doesn't end up including opengl 2 instead of 3
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#endif