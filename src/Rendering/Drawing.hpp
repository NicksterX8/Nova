#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <SDL2/SDL.h>
#include "../GameState.hpp"
#include "../GameViewport.hpp"
#include "../sdl.hpp"
#include "../Camera.hpp"
#include "utils.hpp"

namespace Draw {
    void thickRect(SDL_Renderer* renderer, const SDL_FRect* rect, int thickness);

    void debugRect(SDL_Renderer* renderer, const SDL_FRect* rect, float scale);

    void debugHLine(SDL_Renderer* renderer, float y, float x1, float x2, float scale);

    void debugVLine(SDL_Renderer* renderer, float x, float y1, float y2, float scale);

    void player(const Player* player, SDL_Renderer* renderer, Vec2 offset);

    // @return The number of lines drawn on the screen
    int drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport);

    struct ColoredPoint {
        glm::vec3 position;
        glm::vec4 color;
    };

    static_assert(sizeof(ColoredPoint) == sizeof(GLfloat) * 7, "no struct padding");

    inline void coloredPoints(Camera& camera, GLuint count, ColoredPoint* points, GLfloat pointSize) {
        static VertexAttribute attributes[] = {
            {3, GL_FLOAT, sizeof(GLfloat)},
            {4, GL_FLOAT, sizeof(GLfloat)}
        };
        static ModelData pointModel = makeModel(
            0, NULL, GL_STREAM_DRAW,
            attributes, sizeof(attributes) / sizeof(VertexAttribute)
        );

        glBindVertexArray(pointModel.VAO);
        glBindBuffer(GL_ARRAY_BUFFER, pointModel.VBO);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(ColoredPoint), points, GL_STREAM_DRAW);
        glPointSize(pointSize);
        glDrawArrays(GL_POINTS, 0, count);
    }
}

#endif