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

using GuiRenderLevel = int;
constexpr GuiRenderLevel GuiNumLevels = 16;

struct RenderBufferLevel {
    My::Vec<QuadRenderer::Quad> quads;
    My::Vec<TextRenderBatch> text;
};

constexpr int GuiNullRenderLevel = -1;

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;
    GuiRenderLevel defLevel;
    TextureAtlas guiAtlas;
    RenderOptions options;

    My::Vec<RenderBufferLevel> levels;

    using Color = SDL_Color;

    GuiRenderer() {}

    GuiRenderer(QuadRenderer* quadRenderer, TextRenderer* textRenderer, TextureAtlas guiAtlas, RenderOptions options)
    : quad(quadRenderer), text(textRenderer), defLevel(0), guiAtlas(guiAtlas), options(options) {
        levels = My::Vec<RenderBufferLevel>::WithCapacity(GuiNumLevels);
        for (int i = 0; i < GuiNumLevels; i++) {
            RenderBufferLevel level = {
                My::Vec<QuadRenderer::Quad>::Empty(),
                My::Vec<TextRenderBatch>::Empty()
            };
            levels.push(level);
        }
    }

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

    RenderBufferLevel& getLevel(int level) {
        if (level == GuiNullRenderLevel) {
            level = defLevel;
        } else if (level >= GuiNumLevels) {
            LogError("Level too high!");
            level = GuiNumLevels-1;
        } else if (level < 0) {
            LogError("Invalid level!");
            level = 0;
        }
        return levels[level];
    }

    void setQuadLevel(GuiRenderLevel level) {
        quad->buffer = &getLevel(level).quads;
    }

    void setTextLevel(GuiRenderLevel level) {
        text->buffer = &getLevel(level).text;
    }

    void setLevel(GuiRenderLevel level) {
        setQuadLevel(level);
        setTextLevel(level);
    }

    void renderQuad(const QuadRenderer::Quad& q, int level = GuiNullRenderLevel) {
        setQuadLevel(level);
        quad->render(q);
    }

    struct TextRenderCommand {
        const char* text;
        int textLength;
        glm::vec2 pos;
        TextFormattingSettings formatSettings;
        TextRenderingSettings renderSettings;
    };

    /*
    void renderText(const TextRenderCommand& command, int level) {
        text->buffer = &getLevel(level).text;
        text->render(command);
    }
    */
    void renderText(const char* message, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings, GuiRenderLevel level = GuiNullRenderLevel) {
        setTextLevel(level);
        text->render(message, pos, formatSettings, renderSettings);
    }

    // texture will only be rendered if it's a gui type texture
    void sprite(TextureID texture, const FRect& rect, GuiRenderLevel level = GuiNullRenderLevel) {
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
        renderQuad(q, level);
    }

    struct ColorRect {
        FRect rect;
        Color color;
    };

    void colorRect(FRect rect, Color color, GuiRenderLevel level = GuiNullRenderLevel) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{rect.x, rect.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x, rect.y + rect.h, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y + rect.h, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y, 0}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, level);
    }

    void colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color, GuiRenderLevel level = GuiNullRenderLevel) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{min.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, min.y, 0}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, level);
    }

    void texRect(glm::vec2 min, glm::vec2 max, QuadRenderer::TexCoord texCoord, GuiRenderLevel level = GuiNullRenderLevel) {
        constexpr SDL_Color color = {255, 255, 255, 255};
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, texCoord},
            {glm::vec3{min.x, max.y, 0}, color, texCoord},
            {glm::vec3{max.x, max.y, 0}, color, texCoord},
            {glm::vec3{max.x, min.y, 0}, color, texCoord}
        }};

        renderQuad(_quad, level);
    }

    void rectOutline(glm::vec2 min, glm::vec2 max, Color color, float strokeIn, float strokeOut, GuiRenderLevel level = GuiNullRenderLevel) {
        colorRect({min.x - strokeOut, min.y - strokeOut}, {max.x - strokeIn,  min.y + strokeIn},  color, level);
        colorRect({max.x - strokeIn,  min.y - strokeOut}, {max.x + strokeOut, max.y - strokeIn},  color, level);
        colorRect({max.x + strokeOut, max.y - strokeIn},  {min.x + strokeIn,  max.y + strokeOut}, color, level);
        colorRect({min.x - strokeOut, max.y + strokeOut}, {min.x + strokeIn,  min.y + strokeIn},  color, level);
    }

    void rectOutline(FRect rect, Color color, float strokeIn, float strokeOut, GuiRenderLevel level = GuiNullRenderLevel) {
        glm::vec2 min = {rect.x, rect.y};
        glm::vec2 max = {rect.x + rect.w, rect.y + rect.h};
        rectOutline(min, max, color, strokeIn, strokeOut, level);
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

        for (int i = 0; i < GuiNumLevels; i++) {
            setQuadLevel(i);
            quad->flush(quadShader, transform, guiAtlas.unit);
            setTextLevel(i);
            text->flush(transform);
        }
    }

    void destroy() {
        
    }
};

#endif