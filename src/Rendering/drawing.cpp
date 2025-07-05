#include "rendering/drawing.hpp"
#include "utils/Debug.hpp"
#include "PlayerControls.hpp"

vao_vbo_t Draw::makePointVertexAttribArray() {
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

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, const SDL_Color* colors, const GLfloat* lineWidths) {
    QuadRenderer::Quad* quads = renderer.renderManual(numLines);
    for (GLuint i = 0; i < numLines; i++) {
        GLuint ind = i * 4;
        glm::vec2 p1 = points[i*2];
        glm::vec2 p2 = points[i*2+1];
        glm::vec2 d = p2 - p1;
        glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidths[i] / 2.0f;

        for (int j = 0; j < 4; j++) {
            quads[ind][j].color = colors[i];
            quads[ind][j].texCoord = {0, 0};
        }
        
        quads[ind][0].pos = {p1 + offset, z};
        quads[ind][1].pos = {p1 - offset, z};
        quads[ind][2].pos = {p2 - offset, z};
        quads[ind][3].pos = {p2 + offset, z};
    }
}

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, SDL_Color color, const GLfloat* lineWidths) {
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
        
        quads[ind][0].pos = {p1 + offset, z};
        quads[ind][1].pos = {p1 - offset, z};
        quads[ind][2].pos = {p2 - offset, z};
        quads[ind][3].pos = {p2 + offset, z};
    }
}

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec2* points, float z, SDL_Color color, GLfloat lineWidth) {
    QuadRenderer::Quad* quads = renderer.renderManual(numLines);
    for (GLuint i = 0; i < numLines; i++) {
        glm::vec2 p1 = points[i*2];
        glm::vec2 p2 = points[i*2+1];
        glm::vec2 d = p2 - p1;
        glm::vec2 offset = glm::normalize(glm::vec2(-d.y, d.x)) * lineWidth / 2.0f;

        for (int j = 0; j < 4; j++) {
            quads[i][j].color = color;
            quads[i][j].texCoord = QuadRenderer::NullCoord;
        }
        
        quads[i][0].pos = {p1 + offset, z};
        quads[i][1].pos = {p1 - offset, z};
        quads[i][2].pos = {p2 - offset, z};
        quads[i][3].pos = {p2 + offset, z};
    }
}

int Draw::chunkBorders(QuadRenderer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z) {
    Boxf cameraBounds = camera.maxBoundingArea();
    glm::ivec2 minChunkPos = toChunkPosition(cameraBounds[0]);
    glm::ivec2 maxChunkPos = toChunkPosition(cameraBounds[1]);

    int numLines = maxChunkPos.x - minChunkPos.x + maxChunkPos.y - minChunkPos.y + 2;
    int lineIndex = 0;

    assert(numLines >= 0);
    glm::vec2* points = new glm::vec2[numLines*2];
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        points[lineIndex*2] = {cameraBounds[0].x, y * CHUNKSIZE};
        points[lineIndex*2+1] = {cameraBounds[1].x, y * CHUNKSIZE};
        lineIndex++;
    }
    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        points[lineIndex*2] = {x * CHUNKSIZE, cameraBounds[0].y};
        points[lineIndex*2+1] = {x * CHUNKSIZE, cameraBounds[1].y};
        lineIndex++;
    }

    Draw::thickLines(renderer, numLines, points, z, color, pixelLineWidth / camera.worldScale());
    delete[] points;

    return numLines;
}

void Draw::drawFpsCounter(GuiRenderer& renderer, float fps, float tps, RenderOptions options) {
    auto font = Fonts->get("Debug");
    
    static char fpsCounter[128];

    // only update every 5 frames to not be annoying
    if (Metadata->getFrame() % 5) {
        snprintf(fpsCounter, 128, "FPS: %.1f", fps);
    }

    // if fps is negative or NaN somehow, it will be black
    SDL_Color color = {0,0,0,255};
    int target = Metadata->frame.targetUpdatesPerSecond;
    if (fps > target - 5) {
        color = {0, 255, 0, 255};
    } else if (fps > target - 15) {
        color = {0, 200, 0, 255};
    } else if (fps > target - 30) {
        color = {205, 150, 0, 255};
    } else if (fps > target - 45) {
        color = {235, 60, 0, 255};
    } else if (fps > target - 55) {
        color = {255, 0, 0, 255};
    }
    
    auto fpsRenderInfo = renderer.text->render(fpsCounter, {0, options.size.y}, 
        TextFormattingSettings{.align = TextAlignment::TopLeft}, 
        TextRenderingSettings{.font = font, .color = color, .scale = 1});

    {
        static char tpsCounter[128];

        if (Metadata->getFrame() % 5) {
            snprintf(tpsCounter, 128, "TPS: %.1f", tps);
        }

        // if fps is negative or NaN somehow, it will be black
        SDL_Color color = {0,0,0,255};
        int target = Metadata->tick.targetUpdatesPerSecond;
        if (tps > target - 5) {
            color = {0, 255, 0, 255};
        } else if (tps > target - 15) {
            color = {0, 200, 0, 255};
        } else if (tps > target - 30) {
            color = {205, 150, 0, 255};
        } else if (tps > target - 45) {
            color = {235, 60, 0, 255};
        } else if (tps > target - 55) {
            color = {255, 0, 0, 255};
        }
        
        float offset = fpsRenderInfo.rect.x + fpsRenderInfo.rect.w + 2 * font->advance(' ');
        renderer.text->render(tpsCounter, {offset, options.size.y}, 
            TextFormattingSettings{.align = TextAlignment::TopLeft}, 
            TextRenderingSettings{.font = font, .color = color, .scale = 1.0f});
    }
}

void renderFontComponents(const Font* font, glm::vec2 p, GuiRenderer& renderer) {
    if (!font) return;
    auto* face = font->face;
    if (!face) return;

    //auto height = face->height >> 6;
    //auto ascender = face->ascender >> 6;
    //auto descender = face->descender >> 6;
    auto height = font->height();
    auto ascender = font->ascender();
    auto descender = font->descender();
    auto bbox = face->bbox;
    bbox.xMin >>= 6;
    bbox.xMax >>= 6;
    bbox.yMin >>= 6;
    bbox.yMax >>= 6;

    height *= renderer.options.scale;
    ascender *= renderer.options.scale;
    descender *= renderer.options.scale;

    glm::vec2 points[] = {
        {p.x, p.y},
        {p.x, p.y + descender},
        {p.x, p.y + descender},
        {p.x, p.y + descender + height},
        {p.x, p.y + descender + height},
        {p.x, p.y + descender + height + ascender}
    };

    SDL_Color colors[] = {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255}
    };

    float lineWidths[] = {
        3.0f,
        3.0f,
        3.0f
    };

    FRect bounds = {
        (float)bbox.xMin + p.x,
        (float)bbox.yMin + p.y,
        (float)bbox.xMax - bbox.xMin,
        (float)bbox.yMax - bbox.yMin
    };
    //renderer.colorRect(addBorder(bounds, glm::vec2(0.0f)), {255, 0, 0, 100});
    
    //Draw::thickLines(*renderer.quad, sizeof(points) / (2 * sizeof(glm::vec3)), points, 0.0f, colors, lineWidths);

    //renderer.rectOutline(bounds, {0, 255, 0, 100}, 2.0f, 2.0f);
    //renderer.colorRect(addBorder(bounds, glm::vec2(0.0f)), {255, 0, 0, 100});

    auto result3 = renderer.text->render("This is some!\nThis is some words a\n three now! \n4\n\t5\t\n6\t", renderer.options.size * 0.5f, TextFormattingSettings{.align = TextAlignment::MiddleCenter}, TextRenderingSettings{.font = font, .scale = 2.0f});
    renderer.colorRect(result3.rect, {100, 0, 0, 105});

    auto result2 = renderer.text->render("This is some words!\nLine number 2!!\n three now! \n4\n\t5\t\n6\t", renderer.options.size, TextFormattingSettings{.align = TextAlignment::TopRight}, TextRenderingSettings{.font = font, .scale = 0.5f});
    renderer.colorRect(result2.rect, {100, 0, 0, 105});

    auto result = renderer.text->render("This is one line", {100, 400}, TextFormattingSettings{.align = TextAlignment::BottomLeft}, TextRenderingSettings{.font = font});
    renderer.colorRect(result.rect, {100, 0, 0, 105});
    unsigned int advance = font->advance('T');
    //auto ascender = font->ascender();
    //auto descender = font->descender();
    glm::vec2 advline[] = {
        {result.rect.x, result.rect.y},
        {result.rect.x + advance, result.rect.y}
    };
    //Draw::thickLines(*renderer.quad, 1, advline, 0.5f, SDL_Color{255, 0, 255, 255}, 5.0f);

    glm::vec2 ascline[] = {
        {result.rect.x, result.rect.y + result.rect.h},
        {result.rect.x, result.rect.y + result.rect.h - ascender}
    };
    Draw::thickLines(*renderer.quad, 1, ascline, 0.5f, SDL_Color{155, 255, 255, 255}, 5.0f);

    glm::vec2 descline[] = {
        {result.rect.x, result.rect.y},
        {result.rect.x, result.rect.y - descender}
    };
    Draw::thickLines(*renderer.quad, 1, descline, 0.5f, SDL_Color{255, 0, 155, 255}, 5.0f);
}

// static void testTextRendering(GuiRenderer& guiRenderer, TextRenderer& textRenderer) {
    
   
//     // auto rect2 = textRenderer.render("CENTERING!\n\nthis\n is moreFKDSJLFIKJSDLF\n MANNN \n THis STINKKKKKS\nmin font, mag render scale 435543 gf\nd rer df", {guiRenderer.options.size.x / 2, guiRenderer.options.size.y / 2},
//     //     TextFormattingSettings{.align = TextAlignment::MiddleCenter, .maxWidth = 400}, TextRenderingSettings{.scale = glm::vec2{guiRenderer.textScale() * 2}});
//     //textRenderer.font->scale(1.0f);
    

//     renderFontComponents(guiRenderer.text->defaultFont, {100, 100}, guiRenderer);

//     glm::vec2 points[4] = {
//         {0, guiRenderer.options.size.y/2},
//         {guiRenderer.options.size.x, guiRenderer.options.size.y/2},
//         {guiRenderer.options.size.x/2, 0},
//         {guiRenderer.options.size.x/2, guiRenderer.options.size.y},
//     };
//     Draw::thickLines(*guiRenderer.quad, 2, points, 0.0f, SDL_Color{0, 255, 0, 255}, 2.0f);
// }

void Draw::drawGui(RenderContext& ren, const Camera& camera, const glm::mat4& screenTransform, GUI::Gui* gui, const GameState* state, const PlayerControls& playerControls) {
    auto& textRenderer = ren.guiTextRenderer;
    //textRenderer.setFont(&ren.debugFont);

    GuiRenderer& guiRenderer = ren.guiRenderer;
    auto quadShader = ren.shaders.get(Shaders::Quad);
    quadShader.use();
    quadShader.setMat4("transform", screenTransform);

    auto textShader = ren.shaders.get(Shaders::SDF);
    textShader.use();
    textShader.setMat4("transform", screenTransform);

    gui->draw(guiRenderer, {0, 0, guiRenderer.options.size.x, guiRenderer.options.size.y}, &state->player, state->itemManager, playerControls);

    //textRenderer.setFont(&ren.font);
    Draw::drawFpsCounter(guiRenderer, (float)Metadata->fps(), (float)Metadata->tps(), guiRenderer.options);

    renderFontComponents(Fonts->get("Debug"), {500, 500}, guiRenderer);

    if (Global.paused) {
        guiRenderer.textBox("Paused.",
            Box::Centered({guiRenderer.options.size / 2.0f}, {400, 400}),
            {10, 10}, {200,200,200,140}, 
            {4, 4}, {0,0,0,255},
            TextFormattingSettings{.align = TextAlignment::MiddleCenter}, TextRenderingSettings{.color = {0,0,0,255}}, 10);
    }

    guiRenderer.flush(ren.shaders, screenTransform);
    
}
