#ifndef CAMERA_INCLUDED
#define CAMERA_INCLUDED

#include <math.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    float pixelWidth;
    float pixelHeight;

    float baseScale;
    float zoom;
    
    glm::vec3 position;
    float rotation;

    SDL_Rect displayViewport;

    Camera() {}

    Camera(float baseUnitScale, glm::vec3 pos, int pixelWidth, int pixelHeight, float rotation = 0.0f)
    : pixelWidth(pixelWidth), pixelHeight(pixelHeight), baseScale(baseUnitScale),
    zoom(1.0f), position(pos), rotation(rotation), displayViewport({0, 0, pixelWidth, pixelHeight}) {

    }

    inline float scale() const {
        return baseScale * zoom;
    }

    inline float aspectRatio() const {
        return pixelWidth / pixelHeight;
    }

    inline float viewWidth() const {
        return pixelWidth / scale();
    }

    inline float viewHeight() const {
        return pixelHeight / scale();
    }

    glm::vec2 pixelToWorld(glm::vec2 pixel) const {
        glm::vec2 scaled = {(pixel.x - pixelWidth/2.0f) / scale() * 2, -(pixel.y - pixelHeight/2.0f) / scale() * 2};
        float s = sin(glm::radians(rotation));
        float c = cos(glm::radians(rotation));
        glm::vec2 rotated = {scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c};
        return rotated + glm::vec2(position);
    }

    inline glm::vec2 pixelToWorld(int x, int y) const {
        return pixelToWorld({x, y});
    }

    glm::vec2 worldToPixel(glm::vec2 worldPos) const {
        glm::vec2 delta = worldPos - glm::vec2(position);
        // TODO: do rotation
        glm::vec2 scaled = delta * scale() / 2.0f;
        return scaled + glm::vec2(position);
    }

    inline glm::vec2 minCorner() const {
        return pixelToWorld({0, pixelHeight});
    }

    glm::vec2 maxCorner() const {
        glm::vec2 scaled = {viewWidth(), viewHeight()};
        float s = sin(glm::radians(rotation));
        float c = cos(glm::radians(rotation));
        auto rotated = glm::vec2(scaled.x * c - scaled.y * s, scaled.x * s + scaled.y * c);
        return glm::vec2(position.x, position.y) + rotated;
    }

    glm::mat4 getTransformMatrix() const {
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::scale(trans, glm::vec3(scale() / pixelWidth, scale() / pixelHeight, -1)); // invert z axis so positive z is towards the camera and negative away
        trans = glm::rotate(trans, glm::radians(-rotation), glm::vec3(0.0, 0.0, 1.0));
        trans = glm::translate(trans, -position);
        
        return trans;
    }
};

#endif