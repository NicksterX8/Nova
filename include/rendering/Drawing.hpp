#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <SDL2/SDL.h>
#include "../GameState.hpp"
#include "../sdl.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "text.hpp"
#include "context.hpp"
#include "GUI/Gui.hpp"
#include "rendering/gui.hpp"
#include "renderers.hpp"

namespace Draw {

    /* Types */

    int chunkBorders(ColorQuadRenderBuffer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z);
    void drawFpsCounter(TextRenderer& renderer, float fps, RenderOptions options);
    void drawConsole(GuiRenderer& renderer, const GUI::Console* console);
    void drawGui(RenderContext& ren, const Camera& camera, const glm::mat4& screenTransform, GUI::Gui* gui, const GameState* state);

    struct ColoredPoint {
        glm::vec3 position;
        SDL_Color color;
        float size;
    };

    static_assert(sizeof(ColoredPoint) == sizeof(GLfloat) * 3 + sizeof(SDL_Color) + sizeof(float), "no struct padding");

    inline vao_vbo_t makePointVertexAttribArray() {
        unsigned int vbo,vao;
        glGenBuffers(1, &vbo);
        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);

        constexpr GLsizei stride = sizeof(ColoredPoint);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(0);
        // color
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        // point size
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat) + sizeof(SDL_Color)));
        glEnableVertexAttribArray(2);

        return {vao, vbo};
    }

    inline void coloredPoints(GLsizei count, const ColoredPoint* points) {
        static auto bufferObjects = makePointVertexAttribArray();

        glBindVertexArray(bufferObjects.vao);
        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects.vbo);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(ColoredPoint), points, GL_STREAM_DRAW);

        glDrawArrays(GL_POINTS, 0, count);
    }

    inline void coloredQuads(ColorQuadRenderBuffer& renderer, GLuint count, const ColorVertex* quads) {
       renderer.buffer(count, quads);
    }

    inline void thickLines(ColorQuadRenderBuffer& renderer, GLuint numLines, const glm::vec3* points, const SDL_Color* colors, const GLfloat* lineWidths) {
        ColorVertex* quadPoints = renderer.bufferManual(numLines);
        for (GLint i = 0; i < numLines; i++) {
            GLint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::vec2(-d.y, d.x) * lineWidths[i] / 2.0f;

            for (int j = 0; j < 4; j++) {
                quadPoints[ind+j].color = colors[i];
            }
            
            quadPoints[ind+0].position = {p1 + offset, points[ind+0].z};
            quadPoints[ind+1].position = {p1 - offset, points[ind+0].z};
            quadPoints[ind+2].position = {p2 - offset, points[ind+1].z};
            quadPoints[ind+3].position = {p2 + offset, points[ind+1].z};
        }
    }

    inline void thickLines(ColorQuadRenderBuffer& renderer, GLuint numLines, const glm::vec3* points, SDL_Color color, GLfloat lineWidth) {
        ColorVertex* quadPoints = renderer.bufferManual(numLines);
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidth / 2.0f;

            for (int j = 0; j < 4; j++) {
                quadPoints[ind+j].color = color;
            }
            
            quadPoints[ind+0].position = {p1 + offset, points[i*2].z};
            quadPoints[ind+1].position = {p1 - offset, points[i*2].z};
            quadPoints[ind+2].position = {p2 - offset, points[i*2+1].z};
            quadPoints[ind+3].position = {p2 + offset, points[i*2+1].z};
        }
    }
    
}

using QuadRenderer = ColorQuadRenderBuffer;

#endif