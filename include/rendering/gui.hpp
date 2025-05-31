#ifndef RENDERING_GUI_INCLUDED
#define RENDERING_GUI_INCLUDED

#include "text.hpp"
#include "utils.hpp"
#include "renderers.hpp"
#include "shaders.hpp"

namespace RenderModes {
    enum RenderMode {
        World,
        Screen
    };
}

using RenderModes::RenderMode;

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;
    TextureAtlas guiAtlas;
    RenderOptions options;

    using Color = SDL_Color;

    GuiRenderer() {}

    GuiRenderer(QuadRenderer* quadRenderer, TextRenderer* textRenderer, TextureAtlas guiAtlas, RenderOptions options)
    : quad(quadRenderer), text(textRenderer), guiAtlas(guiAtlas), options(options) {}

    float textScale() const {
        return 1.0f; // scale is already factored into font sizes
    }

    float pixels(float f) const {
        return f * options.scale;
    }

    glm::vec2 pixels(glm::vec2 p) const {
        return p * options.scale;
    }

    QuadRenderer::Quad rectToQuad(const FRect& rect, std::array<QuadRenderer::TexCoord, 4> texCoords) const {
        SDL_Color color = {0,0,0,0};
        const float z = 0.0f;
        QuadRenderer::Quad q = {{
            {{rect.x, rect.y, z}, color, texCoords[0]},
            {{rect.x, rect.y + rect.h, z}, color, texCoords[1]},
            {{rect.x + rect.w, rect.y + rect.h, z}, color, texCoords[2]},
            {{rect.x + rect.w, rect.y, z}, color, texCoords[3]}
        }};
        return q;
    }

    // texture will only be rendered if it's a gui type texture
    void sprite(TextureID texture, const FRect& rect) {
        auto space = getTextureAtlasSpace(&guiAtlas, texture);
        if (!space.valid()) return;
        auto min = space.min;
        auto max = space.max;
        auto q = rectToQuad(rect, {{
            {min.x, min.y},
            {min.x, max.y},
            {max.x, max.y},
            {max.x, min.y}
        }});
        quad->render(q);
    }

    struct ColorRect {
        FRect rect;
        Color color;
    };

    void colorRect(FRect rect, Color color, float z = 0.0f) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{rect.x, rect.y, z}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x, rect.y + rect.h, z}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y + rect.h, z}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y, z}, color, QuadRenderer::NullCoord}
        }};

        quad->render(_quad);
    }

    void colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0.0}, color, QuadRenderer::NullCoord},
            {glm::vec3{min.x, max.y, 0.0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, max.y, 0.0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, min.y, 0.0}, color, QuadRenderer::NullCoord}
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

    void rectOutline(glm::vec2 min, glm::vec2 max, Color color, float strokeIn, float strokeOut) {
        colorRect({min.x - strokeOut, min.y - strokeOut}, {max.x - strokeIn,  min.y + strokeIn},  color);
        colorRect({max.x - strokeIn,  min.y - strokeOut}, {max.x + strokeOut, max.y - strokeIn},  color);
        colorRect({max.x + strokeOut, max.y - strokeIn},  {min.x + strokeIn,  max.y + strokeOut}, color);
        colorRect({min.x - strokeOut, max.y + strokeOut}, {min.x + strokeIn,  min.y + strokeIn},  color);
    }

    void rectOutline(FRect rect, Color color, float strokeIn, float strokeOut) {
        glm::vec2 min = {rect.x, rect.y};
        glm::vec2 max = {rect.x + rect.w, rect.y + rect.h};
        rectOutline(min, max, color, strokeIn, strokeOut);
    }

    float alignmentOffsetFraction(HoriAlignment alignment) const {
        switch (alignment) {
        case HoriAlignment::Left:
            return 0.0f;
        case HoriAlignment::Center:
            return 0.5f;
        case HoriAlignment::Right:
            return 1.0f;
        }
        return NAN;
    }

    void textBox(const My::StringBuffer& textBuffer, int maxLines,
            glm::vec2 pos, TextFormattingSettings formatting, TextRenderingSettings renderingSettings,
            Color backgroundColor, glm::vec2 padding, glm::vec2 minSize = {0,0},
            My::Vec<SDL_Color> textColors = My::Vec<SDL_Color>::Empty()
        ) {

        if (textBuffer.empty()) return;

        // some space is used by padding, take it from the text space
        formatting.maxWidth -= padding.x*2.0f;
        formatting.maxHeight -= padding.y*2.0f;

        formatting.wrapOnWhitespace = true;
        FRect textRect = text->renderStringBufferAsLinesReverse(
            textBuffer, 
            maxLines, // max lines
            pos + padding, // position
            formatting,
            renderingSettings, // scale
            textColors
        );


        auto rect = addBorder(textRect, padding);
        colorRect(rect, backgroundColor);

    }

    void flush(const ShaderManager& shaders, const glm::mat4& transform) {
        auto quadShader = shaders.use(Shaders::Quad);
        quadShader.setVec2("texSize", glm::vec2(guiAtlas.size));
        quad->flush(quadShader, transform, guiAtlas.unit);
        text->flush(transform);
    }

    void destroy() {
        
    }
};

#endif