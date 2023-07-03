#ifndef RENDERING_GUI_INCLUDED
#define RENDERING_GUI_INCLUDED

#include "text.hpp"
#include "utils.hpp"
#include "renderers.hpp"

struct SpriteRenderer {
    
};

struct GuiRenderer {
    QuadRenderer* quad;
    TextRenderer* text;
    SpriteRenderer* sprite;
    RenderOptions options;

    #define CONST(name, val) const auto name = (val * scale)

    GuiRenderer() {}

    GuiRenderer(QuadRenderer* quadRenderer, TextRenderer* textRenderer, RenderOptions options)
    : quad(quadRenderer), text(textRenderer), options(options) {}

    void _sprite(TextureID texture, FRect rect) {
        
    }

    struct ColorRect {
        FRect rect;
        SDL_Color color;
    };

    void colorRect(ColorRect rect) {
        QuadRenderer::UniformQuad2D quad2d;
        quad2d.color = rect.color;
        glm::vec2* positions = quad2d.positions;
        positions[0] = {rect.rect.x,        rect.rect.y,      };
        positions[1] = {rect.rect.x+rect.rect.w, rect.rect.y,      };
        positions[2] = {rect.rect.x+rect.rect.w, rect.rect.y+rect.rect.h};
        positions[3] = {rect.rect.x,        rect.rect.y+rect.rect.h};
        quad->bufferUniform(1, &quad2d);
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

    void flush(glm::mat4 transform, GLuint textTextureUnit) {
        quad->flush();
        text->flush(transform, textTextureUnit);
    }
};

#endif