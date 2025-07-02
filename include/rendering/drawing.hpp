#ifndef DRAWING_INCLUDED
#define DRAWING_INCLUDED

#include <SDL3/SDL.h>
#include "../GameState.hpp"
#include "../sdl.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "text/rendering.hpp"
#include "context.hpp"
#include "GUI/Gui.hpp"
#include "rendering/gui.hpp"
#include "renderers.hpp"
#include "GUI/elements.hpp"

class PlayerControls;

namespace Draw {

    /* Primitve */

    vao_vbo_t makePointVertexAttribArray();

    struct ColoredPoint {
        glm::vec3 position;
        SDL_Color color;
        float size;
    };

    static_assert(sizeof(ColoredPoint) == sizeof(GLfloat) * 3 + sizeof(SDL_Color) + sizeof(float), "no struct padding");

    inline void coloredPoints(GLsizei count, const ColoredPoint* points) {
        static auto bufferObjects = makePointVertexAttribArray();

        glBindVertexArray(bufferObjects.vao);
        glBindBuffer(GL_ARRAY_BUFFER, bufferObjects.vbo);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(ColoredPoint), points, GL_STREAM_DRAW);

        glDrawArrays(GL_POINTS, 0, count);
    }

    void thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, const SDL_Color* colors, const GLfloat* lineWidths);
    void thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, SDL_Color color, const GLfloat* lineWidths);
    void thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, SDL_Color color, GLfloat lineWidth);

    /* GUI */

    int chunkBorders(QuadRenderer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z);
    void drawFpsCounter(GuiRenderer& renderer, float fps, float tps, RenderOptions options);
    void drawGui(RenderContext& ren, const Camera& camera, const glm::mat4& screenTransform, GUI::Gui* gui, const GameState* state, const PlayerControls& playerControls);
    inline void drawItemStack(GuiRenderer& renderer, const ItemManager& itemManager, const ItemStack& itemStack, const FRect& destination) {
        auto displayEc = itemManager.getComponent<ITC::Display>(itemStack.item);
        if (displayEc) {
            auto icon = displayEc->inventoryIcon;
            renderer.sprite(icon, destination);
        }
    }
}

#endif
