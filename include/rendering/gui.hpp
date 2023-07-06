#ifndef RENDERING_GUI_INCLUDED
#define RENDERING_GUI_INCLUDED

#include "text.hpp"
#include "utils.hpp"
#include "renderers.hpp"

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;
    TextureAtlas guiAtlas;
    RenderOptions options;

    #define CONST(name, val) const auto name = (val * scale)

    GuiRenderer() {}

    GuiRenderer(QuadRenderer* quadRenderer, TextRenderer* textRenderer, TextureAtlas guiAtlas, RenderOptions options)
    : quad(quadRenderer), text(textRenderer), guiAtlas(guiAtlas), options(options) {}

    QuadRenderer::Quad rectToQuad(FRect rect, std::array<QuadRenderer::TexCoord, 4> texCoords) {
        SDL_Color color = {0,0,0,0};
        const float z = 0.0f;
        QuadRenderer::Quad q = {{
            {{rect.x, rect.y, z}, color, texCoords[0]},
            {{rect.x, rect.y+rect.h, z}, color, texCoords[1]},
            {{rect.x + rect.w, rect.y + rect.h, z}, color, texCoords[2]},
            {{rect.x + rect.w, rect.h, z}, color, texCoords[3]}
        }};
        return q;
    }   

    // texture will only be rendered if it's a gui type texture
    void guiSprite(TextureID texture, FRect rect) {
        auto space = getTextureAtlasSpace(&guiAtlas, texture);
        if (!space.valid()) return;
        auto min = space.min;
        auto max = space.max;
        auto q = rectToQuad(rect, {{
            {min.x, min.y},
            {min.x, max.y},
            {max.x, max.y},
            {max.x, max.y}
        }});
        quad->render(q);
    }

    void textureArraySprite(TextureArray* textureArray, TextureID texture, FRect rect) {

    }

    struct ColorRect {
        FRect rect;
        SDL_Color color;
    };

    void colorRect(ColorRect rect) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{rect.rect.x, rect.rect.y, 0.0}, rect.color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{rect.rect.x, rect.rect.y + rect.rect.h, 0.0}, rect.color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{rect.rect.x + rect.rect.w, rect.rect.y + rect.rect.h, 0.0}, rect.color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{rect.rect.x + rect.rect.w, rect.rect.y, 0.0}, rect.color, glm::vec<2, GLushort>{0, 0}}
        }};

        quad->render(_quad);
    }

    void colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0.0}, color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{min.x, max.y, 0.0}, color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{max.x, max.y, 0.0}, color, glm::vec<2, GLushort>{0, 0}},
            {glm::vec3{max.x, min.y, 0.0}, color, glm::vec<2, GLushort>{0, 0}}
        }};

        quad->render(_quad);
    }

    void texRect(glm::vec2 min, glm::vec2 max, QuadRenderer::TexCoord texCoord) {
        constexpr SDL_Color color = {255, 255, 255, 255};
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0.0}, color, texCoord},
            {glm::vec3{min.x, max.y, 0.0}, color, texCoord},
            {glm::vec3{max.x, max.y, 0.0}, color, texCoord},
            {glm::vec3{max.x, min.y, 0.0}, color, texCoord}
        }};

        quad->render(_quad);
    }

    void rectOutline(ColorRect rect, float strokeIn, float strokeOut) {
        glm::vec2 min = {rect.rect.x, rect.rect.y};
        glm::vec2 max = {rect.rect.x + rect.rect.w, rect.rect.y + rect.rect.h};
        auto color = rect.color;

        colorRect({min.x - strokeOut, min.y - strokeOut}, {max.x - strokeIn,  min.y + strokeIn},  color);
        colorRect({max.x - strokeIn,  min.y - strokeOut}, {max.x + strokeOut, max.y - strokeIn},  color);
        colorRect({max.x + strokeOut, max.y - strokeIn},  {min.x + strokeIn,  max.y + strokeOut}, color);
        colorRect({min.x - strokeOut, max.y + strokeOut}, {min.x + strokeIn,  min.y + strokeIn},  color);
    }

    // rect.rect.w&h are max sizes, rect.color is background color
    void textBox(ColorRect rect, const My::StringBuffer& textBuffer, int maxLines, My::Vec<SDL_Color> textColors, float textScale = 1.0f) {
        if (textBuffer.empty()) return;
        const float padX = 20 * options.scale;
        const float padY = 20 * options.scale;
        // border and pad need to be the same, change this
        TextFormattingSettings formatting = {
            TextAlignment::BottomLeft,
            rect.rect.w - padX, // max width
        };
        FRect textRect = text->renderStringBufferAsLinesReverse(
            textBuffer, 
            3, // max lines
            glm::vec2(rect.rect.x+padX, rect.rect.y+padY), // position
            textColors,
            glm::vec2(options.scale * textScale), // scale
            formatting
        );
        rect.rect = addBorder(textRect, {padX, padY});
        colorRect(rect);
    }

    void flush(Shader quadShader, Shader textShader, glm::mat4 transform, GLuint textTextureUnit) {
        quad->flush(quadShader, guiAtlas.texture);
        text->flush(textShader, transform, textTextureUnit);
    }

    void destroy() {
        guiAtlas.destroy();
    }
};

#endif