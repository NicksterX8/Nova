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

using GuiRenderHeight = float;

namespace GUI {
    enum class RenderLevel {
        Null = -1,
        Lowest = 0,
        Default = Lowest,
        WorldDebug0,
        WorldDebug1,
        Hotbar,
        HeldItem,
        Console,
        ScreenDebugInfo,
        MenuPause,
        Testing1,
        Testing2,
        Testing3,
        Highest
    };

    using RenderHeight = float;
    constexpr float NullRenderHeight = 0.0f;

    inline RenderHeight getHeight(RenderLevel level) {
        return (RenderHeight)level;
    }
}

using GUI::getHeight;

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;

    TextureAtlas guiAtlas;
    RenderOptions options;

    using Color = SDL_Color;

    GuiRenderer() {}

    GuiRenderer(QuadRenderer* quadRenderer, TextRenderer* textRenderer, TextureAtlas guiAtlas, RenderOptions options)
    : quad(quadRenderer), text(textRenderer), guiAtlas(guiAtlas), options(options) {

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
        QuadRenderer::Quad q = {{
            {{rect.x, rect.y}, color, texCoords[0]},
            {{rect.x, rect.y + rect.h}, color, texCoords[1]},
            {{rect.x + rect.w, rect.y + rect.h}, color, texCoords[2]},
            {{rect.x + rect.w, rect.y}, color, texCoords[3]}
        }};
        return q;
    }

    void renderQuad(const QuadRenderer::Quad& q, GUI::RenderHeight height) {
        quad->render(q, height);
    }

    // struct TextRenderCommand {
    //     const char* text;
    //     int textLength;
    //     glm::vec2 pos;
    //     TextFormattingSettings formatSettings;
    //     TextRenderingSettings renderSettings;
    // };

    /*
    void renderText(const TextRenderCommand& command, int level) {
        text->buffer = &getLevel(level).text;
        text->render(command);
    }
    */
    TextRenderer::RenderResult renderText(const char* message, glm::vec2 pos,
        const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings,
        GUI::RenderHeight height)
    {
        return text->render(message, pos, formatSettings, renderSettings, height);
    }

    TextRenderer::RenderResult renderColoredText(const char* message, ArrayRef<SDL_Color> characterColors, Vec2 pos,
        const TextFormattingSettings& formatSettings, const TextRenderingSettings& renderSettings, 
        GUI::RenderHeight height)
    {
        if (!message) return TextRenderer::RenderResult::BadRender(pos);
        int messageLen = strlen(message);
        return text->renderColored(message, messageLen, characterColors, pos, formatSettings, renderSettings, height);
    }

    // texture will only be rendered if it's a gui type texture
    void sprite(TextureID texture, const FRect& rect, GUI::RenderHeight height) {
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
        renderQuad(q, height);
    }

    struct ColorRect {
        FRect rect;
        Color color;
    };

    void colorRect(FRect rect, Color color, GUI::RenderHeight height) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec2{rect.x, rect.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{rect.x, rect.y + rect.h}, color, QuadRenderer::NullCoord},
            {glm::vec2{rect.x + rect.w, rect.y + rect.h}, color, QuadRenderer::NullCoord},
            {glm::vec2{rect.x + rect.w, rect.y}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, height);
    }

    void colorRect(glm::vec2 min, glm::vec2 max, SDL_Color color, GUI::RenderHeight height) {
        auto _quad = QuadRenderer::Quad{{
            {glm::vec2{min.x, min.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{min.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, max.y}, color, QuadRenderer::NullCoord},
            {glm::vec2{max.x, min.y}, color, QuadRenderer::NullCoord}
        }};

        renderQuad(_quad, height);
    }

    void colorRect(Box box, SDL_Color color, GUI::RenderHeight height) {
        colorRect(box.min, box.max(), color, height);
    }

    void texRect(glm::vec2 min, glm::vec2 max, QuadRenderer::TexCoord texCoord, GUI::RenderHeight height) {
        constexpr SDL_Color color = {255, 255, 255, 255};
        auto _quad = QuadRenderer::Quad{{
            {glm::vec2{min.x, min.y}, color, texCoord},
            {glm::vec2{min.x, max.y}, color, texCoord},
            {glm::vec2{max.x, max.y}, color, texCoord},
            {glm::vec2{max.x, min.y}, color, texCoord}
        }};

        renderQuad(_quad, height);
    }

    void rectOutline(glm::vec2 min, glm::vec2 max, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GUI::RenderHeight height) {
        colorRect({min.x - strokeOut.x, min.y - strokeOut.y}, {max.x -  strokeIn.x, min.y +  strokeIn.y}, color, height);
        colorRect({max.x -  strokeIn.x, min.y - strokeOut.y}, {max.x + strokeOut.x, max.y -  strokeIn.y}, color, height);
        colorRect({max.x + strokeOut.x, max.y -  strokeIn.y}, {min.x +  strokeIn.x, max.y + strokeOut.y}, color, height);
        colorRect({min.x - strokeOut.x, max.y + strokeOut.y}, {min.x +  strokeIn.x, min.y +  strokeIn.y}, color, height);
    }

    void rectOutline(FRect rect, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GUI::RenderHeight height) {
        glm::vec2 min = {rect.x, rect.y};
        glm::vec2 max = {rect.x + rect.w, rect.y + rect.h};
        rectOutline(min, max, color, strokeIn, strokeOut, height);
    }

    void rectOutline(Box box, Color color, glm::vec2 strokeIn, glm::vec2 strokeOut, GUI::RenderHeight height) {
        rectOutline(box.min, box.max(), color, strokeIn, strokeOut, height);
    }

    TextRenderer::RenderResult boxedText(const char* message, Box boundingBox, TextFormattingSettings formatting, TextRenderingSettings renderSettings, GUI::RenderHeight height) {
        formatting.maxWidth = MIN(formatting.maxWidth, boundingBox.size.x);
        formatting.maxHeight = MIN(formatting.maxHeight, boundingBox.size.y);
        return renderText(message, boundingBox.min, formatting, renderSettings, height);
    }

    TextRenderer::RenderResult textBox(const char* message, Box box, Vec2 padding,
        Color backgroundColor, Vec2 borderSize, Color borderColor,
        TextFormattingSettings formatting, TextRenderingSettings renderSettings,
        GUI::RenderHeight height)
    {   
        colorRect(box, backgroundColor, height);
        rectOutline(box, borderColor, {0, 0}, borderSize, height);

        Vec2 textOffset = getAlignmentOffset(formatting.align, box.size);
        
        return boxedText(message, {box.min + padding + textOffset, box.size - padding*2.0f}, formatting, renderSettings, height);
    }

    void flush(const ShaderManager& shaders, const glm::mat4& transform) {
        auto quadShader = shaders.use(Shaders::Quad);
        quadShader.setVec2("texSize", glm::vec2(guiAtlas.size));

        useShader(Shaders::Text).setMat4("transform", transform);
        useShader(Shaders::SDF).setMat4("transform", transform);

        quad->flush(quadShader, transform, guiAtlas.unit);
        text->flushBuffer();
        text->clearBuffers();
    }

    void destroy() {
        
    }
};

#endif