#ifndef GLOBAL_H
#define GLOBAL_H

#include <glm/vec2.hpp>
#include "NC/cpp-vectors.hpp"

#define GLOBAL_FILEPATH_SIZE 512

namespace FilePaths {
    extern char assets[GLOBAL_FILEPATH_SIZE];
    extern char shaders[GLOBAL_FILEPATH_SIZE];
    extern char save[GLOBAL_FILEPATH_SIZE];
}

inline Vec2 toVec2(glm::vec2 vec) {
    return Vec2(vec.x, vec.y);
}

#endif