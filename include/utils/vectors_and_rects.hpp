#ifndef UTILS_VECTORS_AND_RECTS_INCLUDED
#define UTILS_VECTORS_AND_RECTS_INCLUDED

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <math.h>
#include <SDL3/SDL_rect.h>
#include <array>

using Vec2 = glm::vec2;
using IVec2 = glm::ivec2;

struct IVec2Hash {
    size_t operator()(const IVec2& point) const {
        constexpr size_t hashTypeHalfBits = sizeof(size_t) * 8 / 2;
        static_assert(sizeof(point) == sizeof(size_t), "point fits into hash");
        // y coordinate is most significant 32 bits, x least significant 32 bits
        return ((size_t)point.y << hashTypeHalfBits) | ((size_t)point.x); 
    }
};

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

/*
struct Boxi {
    glm::ivec2 min;
    glm::ivec2 max;
};

struct Boxf {
    glm::vec2 min;
    glm::vec2 max;
};
*/

using Boxf = std::array<glm::vec2, 2>; // array with min and max vertices
using Boxi = std::array<glm::ivec2, 2>; // array with min and max vertices

struct Box {
    Vec2 min;
    Vec2 size;

    Vec2 max() const {
        return min + size;
    }

    Vec2 center() const {
        return min + size/2.0f;
    }

    FRect rect() const {
        return FRect{
            min.x,
            min.y,
            size.x,
            size.y
        };
    }
};

inline Vec2 vecFloor(Vec2 vec) {
    return {floor(vec.x), floor(vec.y)};
}

inline IVec2 vecFloori(Vec2 vec) {
    return {(int)floor(vec.x), (int)floor(vec.y)};
}

inline Box* rectAsBox(FRect* box) {
    return (Box*)box;
}

#endif