#ifndef GLOBAL_H
#define GLOBAL_H

#include <glm/vec2.hpp>
#include "NC/cpp-vectors.hpp"

#define GLOBAL_FILEPATH_SIZE 1024

namespace FilePaths {
    // Base path to resources
    extern char base[GLOBAL_FILEPATH_SIZE];
    extern char assets[GLOBAL_FILEPATH_SIZE];
    extern char shaders[GLOBAL_FILEPATH_SIZE]; // path to shaders
    extern char save[GLOBAL_FILEPATH_SIZE]; // path to save folder
}

#endif