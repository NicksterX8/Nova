#ifndef UTILS_VECTORS_INCLUDED
#define UTILS_VECTORS_INCLUDED

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <math.h>
#include <SDL2/SDL_rect.h>

using Vec2 = glm::vec2;
using IVec2 = glm::ivec2;

using Rect = SDL_Rect;
using FRect = SDL_FRect;

inline bool pointInRect(glm::vec2 p, FRect rect) {
    return (
           p.x > rect.x
        && p.y > rect.y
        && p.x < rect.x + rect.w
        && p.y < rect.y + rect.h
    );
}

inline bool pointInRect(glm::vec2 p, Rect rect) {
    return (
           p.x > (float)rect.x
        && p.y > (float)rect.y
        && p.x < (float)(rect.x + rect.w)
        && p.y < (float)(rect.y + rect.h)
    );
}

inline bool pointInRect(IVec2 p, FRect rect) {
    return (
           (float)p.x > rect.x
        && (float)p.y > rect.y
        && (float)p.x < rect.x + rect.w
        && (float)p.y < rect.y + rect.h
    );
}

inline bool pointInRect(IVec2 p, Rect rect) {
    return (
           p.x > rect.x
        && p.y > rect.y
        && p.x < rect.x + rect.w
        && p.y < rect.y + rect.h
    );
}

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