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

    // @return The number of lines drawn on the screen
    int drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport);

    struct ColoredPoint {
        glm::vec3 position;
        glm::vec4 color;
        float size;
    };

    struct ColorVertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    static_assert(sizeof(ColorVertex) == sizeof(GLfloat) * 7, "no struct padding");
    static_assert(sizeof(ColoredPoint) == sizeof(GLfloat) * 8, "no struct padding");

    struct buffersVAOVBO {
        GLuint vao;
        GLuint vbo;
    };

    struct buffersVAOVBOEBO {
        GLuint vao;
        GLuint vbo;
        GLuint ebo;
    };

    inline buffersVAOVBO makePointVertexAttribArray() {
        unsigned int vbo,vao;
        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);

        size_t stride = 8 * sizeof(GLfloat);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // point size
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);

        return {vao, vbo};
    }

    inline void coloredPoints(GLuint count, ColoredPoint* points) {
        static auto bufferObjects = makePointVertexAttribArray();

        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects.vbo);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(ColoredPoint), points, GL_STREAM_DRAW);

        glBindVertexArray(bufferObjects.vao);
        glDrawArrays(GL_POINTS, 0, count);

        checkOpenGLErrors();
    }

    inline buffersVAOVBOEBO makeQuadVertexAttribArray() {
        GLuint vbo,vao,ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);

        size_t stride = 7 * sizeof(GLfloat);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
 
        return {vao, vbo, ebo};
    }

    inline void coloredQuads(GLuint count, ColorVertex* quads) {
        static auto bufferObjects = makeQuadVertexAttribArray();

        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects.vbo);
        glBufferData(GL_ARRAY_BUFFER, count * 4 * sizeof(ColorVertex), quads, GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObjects.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count*6*sizeof(GLuint), NULL, GL_STREAM_DRAW);
        GLuint* indices = static_cast<GLuint*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

        for (GLuint i = 0; i < count; i++) {
            GLuint first = i*4;
            GLuint ind = i * 6;
            indices[ind+0] = first+0;
            indices[ind+1] = first+1;
            indices[ind+2] = first+3;
            indices[ind+3] = first+1;
            indices[ind+4] = first+2;
            indices[ind+5] = first+3; 
        }

        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        glBindVertexArray(bufferObjects.vao);
        glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_INT, 0);

        checkOpenGLErrors();
    }

    inline void lines(GLuint numLines, glm::vec3* points, glm::vec4* colors, GLfloat* lineWidths) {
        ColorVertex* quadPoints = new ColorVertex[numLines*4];
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::vec2(-d.x, d.x) * lineWidths[i] / 2.0f;

            for (int j = 0; j < 4; j++) {
                quadPoints[ind+j].color = colors[i];
            }
            
            quadPoints[ind+0].position = {p1 + offset, points[ind+0].z};
            quadPoints[ind+1].position = {p1 - offset, points[ind+0].z};
            quadPoints[ind+2].position = {p2 - offset, points[ind+1].z};
            quadPoints[ind+3].position = {p2 + offset, points[ind+1].z};
        }

        coloredQuads(numLines, quadPoints);
    }
}

#endif