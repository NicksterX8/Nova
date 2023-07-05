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

    int chunkBorders(QuadRenderer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z);
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

    inline void thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec3* points, SDL_Color color, const GLfloat* lineWidths) {
        QuadRenderer::Quad* quads = renderer.renderManual(numLines);
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidths[i] / 2.0f;

            for (int j = 0; j < 4; j++) {
                quads[ind][j].color = color;
                quads[ind][j].texCoord = {0, 0};
            }
            
            quads[ind][0].pos = {p1 + offset, points[i*2].z};
            quads[ind][1].pos = {p1 - offset, points[i*2].z};
            quads[ind][2].pos = {p2 - offset, points[i*2+1].z};
            quads[ind][3].pos = {p2 + offset, points[i*2+1].z};
        }
    }

    inline void thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec3* points, SDL_Color color, GLfloat lineWidth) {
        QuadRenderer::Quad* quads = renderer.renderManual(numLines);
        for (GLuint i = 0; i < numLines; i++) {
            GLuint ind = i * 4;
            glm::vec2 p1 = points[i*2];
            glm::vec2 p2 = points[i*2+1];
            glm::vec2 d = p2 - p1;
            glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidth / 2.0f;

            for (int j = 0; j < 4; j++) {
                quads[ind][j].color = color;
                quads[ind][j].texCoord = {0, 0};
            }
            
            quads[ind][0].pos = {p1 + offset, points[i*2].z};
            quads[ind][1].pos = {p1 - offset, points[i*2].z};
            quads[ind][2].pos = {p2 - offset, points[i*2+1].z};
            quads[ind][3].pos = {p2 + offset, points[i*2+1].z};
        }
    }
    
}

#endif