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
private:
    float rotation; // in degrees
public:
    float cosine;
    float sine;

    SDL_FRect displayViewport;

    Camera() {}

    Camera(float baseUnitScale, glm::vec3 pos, int pixelWidth, int pixelHeight, float _rotation = 0.0f)
    : pixelWidth(pixelWidth), pixelHeight(pixelHeight), baseScale(baseUnitScale),
    zoom(1.0f), position(pos), rotation(_rotation), displayViewport(FRect{0, 0, (float)pixelWidth, (float)pixelHeight}) {
        auto radians = glm::radians(rotation);
        sine = sin(rotation);
        cosine = cos(rotation);    
    }

    float worldScale() const {
        return baseScale * zoom;
    }

    float aspectRatio() const {
        return pixelWidth / pixelHeight;
    }

    float viewWidth() const {
        return pixelWidth / worldScale();
    }

    float viewHeight() const {
        return pixelHeight / worldScale();
    }

    // camera angle in degrees
    float angle() const {
        return rotation;
    }

    void setAngle(float degrees) {
        if (rotation != degrees) {
            rotation = degrees;
            auto radians = glm::radians(fmod(rotation, 360.0f));
            sine = sin(radians);
            cosine = cos(radians);
        }
    }

    bool pixelOnScreen(glm::vec2 pixel) const {
        int roundX = round(pixel.x);
        int roundY = round(pixel.y);
        return roundX < pixelWidth && roundX >= 0 && roundY < pixelHeight && roundY >= 0;
    }

    glm::vec2 pixelToWorld(glm::vec2 pixel) const {
        // in order: scale, rototate, translate back to pixel postion
        // dont know why we multiply by 2 tbh, but it works
        glm::vec2 scaled = {(pixel.x - pixelWidth/2.0f) / worldScale() * 2, -(pixel.y - pixelHeight/2.0f) / worldScale() * 2};
        glm::vec2 rotated = {scaled.x * cosine - scaled.y * sine, scaled.x * sine + scaled.y * cosine};
        return rotated + glm::vec2(position);
    }

    inline glm::vec2 pixelToWorld(int x, int y) const {
        return pixelToWorld({x, y});
    }

    glm::vec2 worldToPixel(glm::vec2 worldPos) const {
        // do inverse of pixelToWorld
        // distance from camera is needed, not distance from 0,0
        glm::vec2 delta = worldPos - glm::vec2(position);

        // To rotate negative rotation to get back to the screen rotation (0 degrees) we only need to use negative sin,
        // because of the trigonmetric identities, 
        // cos(-r) = cos(r); sin(-r) = -sin(r)
        float c = cosine;
        float s = -sine;

        // use classic rotation matrix to rotate the delta
        glm::vec2 rotated = {delta.x * c - delta.y * s, delta.x * s + delta.y * c};
        // scale world units to pixels
        glm::vec2 scaled = rotated * worldScale();
        // I dont know why we divide the scaled vector by 2 (after its added with the screen size) tbh, but it works
        return (scaled + glm::vec2{pixelWidth, pixelHeight}) / 2.0f;
    }

    /* Camera viewport corner positions in world coordinates */
    glm::vec2 TopLeftWorldPos() const {
        return pixelToWorld({0, pixelHeight});
    }
    glm::vec2 TopRightWorldPos() const {
        return pixelToWorld({pixelWidth, pixelHeight});
    }
    glm::vec2 BottomRightWorldPos() const {
        return pixelToWorld({pixelWidth, 0});
    }
    glm::vec2 BottomLeftWorldPos() const {
        return pixelToWorld({0, 0});
    }
    
    /* TODO: make these more efficient */
    glm::vec2 minCorner() const {
        auto area = maxBoundingArea();
        return area[0];
    }
    glm::vec2 maxCorner() const {
        auto area = maxBoundingArea();
        return area[1];
    }

    // get a rect including all area coverd by the camera, and possibly some extra corners and stuff if the camera is rotated, so the final rect can be larger than actual width and height
    Boxf maxBoundingArea() const {
        const glm::vec2 corners[4] = {
            pixelToWorld({0, 0}),
            pixelToWorld({pixelWidth, 0}),
            pixelToWorld({pixelWidth, pixelHeight}),
            pixelToWorld({0, pixelHeight})
        };
        // bruteforce test all 4 corners to find the lowest and highest coords.
        glm::vec2 min = corners[0];
        glm::vec2 max = corners[0];
        for (int i = 1; i < 4; i++) {
            glm::vec2 corner = corners[i];
            min.x = MIN(min.x, corner.x);
            min.y = MIN(min.y, corner.y);
            max.x = MAX(max.x, corner.x);
            max.y = MAX(max.y, corner.y);
        }
        return {min, max};
    }

    bool rectIsVisible(glm::vec2 p, glm::vec2 q) const {
        const glm::vec2 corners[4] = {
            worldToPixel({p.x, p.y}),
            worldToPixel({q.x, p.y}),
            worldToPixel({q.x, q.y}),
            worldToPixel({p.x, q.y})
        };
        // bruteforce test all 4 corners to find the lowest and highest coords.
        glm::vec2 min = corners[0];
        glm::vec2 max = corners[0];
        for (int i = 1; i < 4; i++) {
            glm::vec2 corner = corners[i];
            min.x = MIN(min.x, corner.x);
            min.y = MIN(min.y, corner.y);
            max.x = MAX(max.x, corner.x);
            max.y = MAX(max.y, corner.y);
        }
        return (min.x < pixelWidth && min.y < pixelHeight && max.x > 0 && max.y > 0);
    }

    glm::mat4 getTransformMatrix() const {
        glm::mat4 trans = glm::mat4(1.0f);
        trans = glm::scale(trans, glm::vec3(worldScale() / pixelWidth, worldScale() / pixelHeight, -1)); // invert z axis so positive z is towards the camera and negative away
        // rotate everything in view away from the camera because it's relative yaknow. rotate around the z axis cause x and y are the ones we're looking at and want to move
        trans = glm::rotate(trans, glm::radians(-rotation), glm::vec3(0.0, 0.0, 1.0));
        // translate everything in view away from the camera
        trans = glm::translate(trans, -position);
        
        return trans;
    }
};

#endif