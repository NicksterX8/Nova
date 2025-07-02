#ifndef RENDERING_GUI_INCLUDED
#define RENDERING_GUI_INCLUDED

#include "text/rendering.hpp"
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

constexpr GuiRenderLevel GuiNullRenderLevel = -1;

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;

    static constexpr GuiRenderLevel UseDefaultLevel = -2;
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

    RenderBufferLevel* getLevel(GuiRenderLevel level) {
        if (level == GuiNullRenderLevel) {
            return nullptr;
        } else if (level == UseDefaultLevel) {
            level = defLevel;
        } else if (level >= GuiNumLevels) {
            LogError("Level too high!");
            return nullptr;
        } else if (level < 0) {
            LogError("Invalid level!");
            return nullptr;
        }
        return &levels[level];
    }

    void setQuadLevel(GuiRenderLevel level) {
        RenderBufferLevel* levelBuffer = getLevel(level);
        quad->buffer = levelBuffer ? &levelBuffer->quads : nullptr;
    }

    void setTextLevel(GuiRenderLevel level) {
        RenderBufferLevel* levelBuffer = getLevel(level);
        text->buffer = levelBuffer ? &levelBuffer->text : nullptr;
    }

    void setLevel(GuiRenderLevel level) {
        setQuadLevel(level);
        setTextLevel(level);
    }

    void renderQuad(const QuadRenderer::Quad& q, GuiRenderLevel level = UseDefaultLevel) {
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
    TextRenderer::RenderResult renderText(const char* message, glm::vec2 pos, const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings, GuiRenderLevel level = UseDefaultLevel, Vec2* characterPositions = nullptr) {
        setTextLevel(level);
        return text->render(message, pos, formatSettings, renderSettings, characterPositions);
    }

    // texture will only be rendered if it's a gui type texture
    void sprite(TextureID texture, const FRect& rect, GuiRenderLevel level = UseDefaultLevel) {
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

    void colorRect(FRect rect, Color color, GuiRenderLevel level = UseDefaultLevel) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{rect.x, rect.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x, rect.y + rect.h, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y + rect.h, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{rect.x + rect.w, rect.y, 0}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, level);
    }

    void colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color, GuiRenderLevel level = UseDefaultLevel) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{min.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, max.y, 0}, color, QuadRenderer::NullCoord},
            {glm::vec3{max.x, min.y, 0}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, level);
    }

    void colorRect(Box box, SDL_Color color, GuiRenderLevel level = UseDefaultLevel) {
        colorRect(box.min, box.max(), color, level);
    }

    void texRect(glm::vec2 min, glm::vec2 max, QuadRenderer::TexCoord texCoord, GuiRenderLevel level = UseDefaultLevel) {
        constexpr SDL_Color color = {255, 255, 255, 255};
        auto _quad = QuadRenderer::Quad{{
            {glm::vec3{min.x, min.y, 0}, color, texCoord},
            {glm::vec3{min.x, max.y, 0}, color, texCoord},
            {glm::vec3{max.x, max.y, 0}, color, texCoord},
            {glm::vec3{max.x, min.y, 0}, color, texCoord}
        }};

        renderQuad(_quad, level);
    }

    void rectOutline(glm::vec2 min, glm::vec2 max, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GuiRenderLevel level = UseDefaultLevel) {
        colorRect({min.x - strokeOut.x, min.y - strokeOut.y}, {max.x -  strokeIn.x, min.y +  strokeIn.y}, color, level);
        colorRect({max.x -  strokeIn.x, min.y - strokeOut.y}, {max.x + strokeOut.x, max.y -  strokeIn.y}, color, level);
        colorRect({max.x + strokeOut.x, max.y -  strokeIn.y}, {min.x +  strokeIn.x, max.y + strokeOut.y}, color, level);
        colorRect({min.x - strokeOut.x, max.y + strokeOut.y}, {min.x +  strokeIn.x, min.y +  strokeIn.y}, color, level);
    }

    void rectOutline(FRect rect, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GuiRenderLevel level = UseDefaultLevel) {
        glm::vec2 min = {rect.x, rect.y};
        glm::vec2 max = {rect.x + rect.w, rect.y + rect.h};
        rectOutline(min, max, color, strokeIn, strokeOut, level);
    }

    void rectOutline(Box box, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GuiRenderLevel level = UseDefaultLevel) {
        rectOutline(box.min, box.max(), color, strokeIn, strokeOut, level);
    }

    TextRenderer::RenderResult boxedText(const char* message, Box boundingBox, TextFormattingSettings formatting, TextRenderingSettings renderSettings, GuiRenderLevel level = UseDefaultLevel, Vec2* characterPositions = nullptr) {
        formatting.maxWidth = MIN(formatting.maxWidth, boundingBox.size.x);
        formatting.maxHeight = MIN(formatting.maxHeight, boundingBox.size.y);
        return renderText(message, boundingBox.min, formatting, renderSettings, level, characterPositions);
    }

    TextRenderer::RenderResult textBox(const char* message, Box box, Vec2 padding,
        Color backgroundColor, Vec2 borderSize, Color borderColor,
        TextFormattingSettings formatting, TextRenderingSettings renderSettings,
        GuiRenderLevel level = UseDefaultLevel)
    {   
        colorRect(box, backgroundColor, level);
        rectOutline(box, borderColor, {0, 0}, borderSize, level);

        Vec2 textOffset = getAlignmentOffset(formatting.align, box.size);
        
        return boxedText(message, {box.min + padding + textOffset, box.size - padding*2.0f}, formatting, renderSettings, level);
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