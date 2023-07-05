#ifndef GL_INCLUDED
#define GL_INCLUDED

#define GL_SILENCE_DEPRECATION 1 // openGL is deprecated on macos and makes tons of warnings unless silenced with this.
#include <OpenGL/gl3.h> // this needs to be included before sdl opengl is so it doesn't end up including opengl 2 instead of 3
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_image.h>
#include "utils/Log.hpp"
#include "utils/vectors.hpp"

static_assert(sizeof(GLfloat) == sizeof(float), "float type is as expected");
static_assert(sizeof(GLuint) == sizeof(unsigned int), "unsigned int type is as expected");

template<int Length>
using GLvec = glm::vec<Length, GLfloat>;
using GLvec2 = GLvec<2>;
using GLvec3 = GLvec<3>;
using GLvec4 = GLvec<4>;

namespace GL {

inline int logErrors() {
    int i;
    int numErrors = 0;
    GLenum errors[256];
    while (true) {
        errors[numErrors] = glGetError();
        if (errors[numErrors] == GL_NO_ERROR) break;
        numErrors++;
    }

    for (i = 0; i < numErrors; i++) {
        const char* message = "";
        switch (errors[i]) {
        case GL_INVALID_VALUE:
            message = "Invalid value";
            break;
        case GL_INVALID_ENUM:
            message = "Invalid enum";
            break;
        case GL_INVALID_OPERATION:
            message = "Invalid operation";
            break;
        case GL_INVALID_INDEX:
            message = "Invalid index";
            break;
        case GL_OUT_OF_MEMORY:
            message = "Out of memory";
            break;
        case GL_STACK_OVERFLOW:
            message = "stack overflow";
            break;
        case GL_STACK_UNDERFLOW:
            message = "stack underflow";
            break;
        }
        LogError("OpenGL error: %d, %s", errors[i], message);
    }

    return numErrors;
}

}

#endif