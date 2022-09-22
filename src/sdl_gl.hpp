#ifndef SDL_GL_INCLUDED
#define SDL_GL_INCLUDED

#define GL_SILENCE_DEPRECATION 1 // openGL is deprecated on macos and makes tons of warnings unless silenced with this.
#include <OpenGL/gl3.h> // this needs to be included before sdl opengl is so it doesn't end up including opengl 2 instead of 3
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "utils/Log.hpp"

static_assert(sizeof(GLfloat) == sizeof(float), "float type is as expected");
static_assert(sizeof(GLuint) == sizeof(unsigned int), "unsigned int type is as expected");

inline int checkOpenGLErrors() {
    int i;
    int numErrors = 0;
    GLenum errors[256];
    while (true) {
        errors[numErrors] = glGetError();
        if (errors[numErrors] == GL_NO_ERROR) break;
        numErrors++;
    }

    for (i = 0; i < numErrors; i++) {
        LogError("OpenGL error: %d", errors[i]);
    }

    return numErrors;
}

#endif