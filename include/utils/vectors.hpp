#ifndef UTILS_VECTORS_INCLUDED
#define UTILS_VECTORS_INCLUDED

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <math.h>

using Vec2 = glm::vec2;
using IVec2 = glm::ivec2;

struct iRect {
    glm::ivec2 pos;
    glm::ivec2 size;
};

struct fRect {
    glm::vec2 pos;
    glm::vec2 size;
};

inline Vec2 vecFloor(Vec2 vec) {
    return {floor(vec.x), floor(vec.y)};
}

inline IVec2 vecFloori(Vec2 vec) {
    return {(int)floor(vec.x), (int)floor(vec.y)};
}

#endif