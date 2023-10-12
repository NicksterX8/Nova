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

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec3* points, const SDL_Color* colors, const GLfloat* lineWidths) {
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
        
        quads[ind][0].pos = {p1 + offset, points[i*2].z};
        quads[ind][1].pos = {p1 - offset, points[i*2].z};
        quads[ind][2].pos = {p2 - offset, points[i*2+1].z};
        quads[ind][3].pos = {p2 + offset, points[i*2+1].z};
    }
}

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec3* points, SDL_Color color, const GLfloat* lineWidths) {
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

void Draw::thickLines(QuadRenderer& renderer, GLuint numLines, const glm::vec3* points, SDL_Color color, GLfloat lineWidth) {
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
        
        quads[i][0].pos = {p1 + offset, points[i*2].z};
        quads[i][1].pos = {p1 - offset, points[i*2].z};
        quads[i][2].pos = {p2 - offset, points[i*2+1].z};
        quads[i][3].pos = {p2 + offset, points[i*2+1].z};
    }
}

int Draw::chunkBorders(QuadRenderer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z) {
    Boxf cameraBounds = camera.maxBoundingArea();
    glm::ivec2 minChunkPos = toChunkPosition(cameraBounds[0]);
    glm::ivec2 maxChunkPos = toChunkPosition(cameraBounds[1]);

    int numLines = maxChunkPos.x - minChunkPos.x + maxChunkPos.y - minChunkPos.y + 2;
    int lineIndex = 0;

    assert(numLines >= 0);
    glm::vec3* points = new glm::vec3[numLines*2];
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        points[lineIndex*2] = glm::vec3(cameraBounds[0].x, y * CHUNKSIZE, z);
        points[lineIndex*2+1] = glm::vec3(cameraBounds[1].x, y * CHUNKSIZE, z);
        lineIndex++;
    }
    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        points[lineIndex*2] = glm::vec3(x * CHUNKSIZE, cameraBounds[0].y, z);
        points[lineIndex*2+1] = glm::vec3(x * CHUNKSIZE, cameraBounds[1].y, z);
        lineIndex++;
    }

    Draw::thickLines(renderer, numLines, points, color, pixelLineWidth / camera.worldScale());
    delete[] points;

    return numLines;
}

void Draw::drawFpsCounter(TextRenderer& renderer, float fps, RenderOptions options) {
    static char fpsCounter[128];

    static float fpsLastFrame = fps;

    if (Metadata->currentFrame() % 5) {
        float averagedFps = (fps + fpsLastFrame) / 2.0f;
        snprintf(fpsCounter, 128, "FPS: %.1f", averagedFps);
    }

    // if fps is negative or NaN somehow, it will be black
    SDL_Color color = {0,0,0,255};
    if (fps > 110.0f) {
        color = {0, 255, 0, 255};
    } else if (fps > 54.0f) {
        color = {0, 200, 0, 255};
     } else if (fps > 36.0f) {
        color = {205, 150, 0, 255};
     } else if (fps > 18.0f) {
        color = {235, 60, 0, 255};
     } else if (fps > 0.0f) {
        color = {255, 0, 0, 255};
     }
    
    renderer.render(fpsCounter, {0, options.size.y}, 
        TextFormattingSettings(TextAlignment::TopLeft), 
        TextRenderingSettings(color, glm::vec2(2.0f)));

    fpsLastFrame = fps;
}

void Draw::drawConsole(GuiRenderer& renderer, const GUI::Console* console) {
    if (!console) return;

    // for everything
    float maxWidth = renderer.options.size.x;
    float maxHeight = renderer.options.size.y;
    // max width for log
    float logMaxWidth = MIN(300 * renderer.options.scale, maxWidth);

    float logOffsetY = 0;
    
    if (console->promptOpen) {
        // Render active message being written
        glm::vec2 activeMsgBorder = renderer.pixels({20.0f, 10.0f});
    
        float activeMsgMinWidth = logMaxWidth;
        float activeMsgMinHeight = renderer.text->lineHeight() * renderer.textScale() + activeMsgBorder.y*2; // always leave room for a line

        constexpr SDL_Color activeMessageBackground = {30, 30, 30, 205};
        glm::vec2 selectedCharPos = {activeMsgBorder.x, activeMsgBorder.y - renderer.text->getFont()->descender()};
        int selectedCharIndex = console->selectedCharIndex;
        FRect activeMessageRect = {0, 0, activeMsgMinWidth, activeMsgMinHeight};
        if (!console->activeMessage.empty()) {
            TextFormattingSettings formatting(TextAlignment::BottomLeft, maxWidth - activeMsgBorder.x, maxHeight - activeMsgBorder.y); // active message max width is only the screen width
            formatting.wrapOnWhitespace = true;
            TextRenderingSettings rendering(GUI::Console::activeTextColor, glm::vec2(renderer.textScale()));
            llvm::SmallVector<glm::vec2> characterPositions(console->activeMessage.size());
            auto textInfo = renderer.text->render(console->activeMessage.c_str(), activeMsgBorder, formatting, rendering, characterPositions.data());
            FRect textRect = textInfo.rect;
            if (selectedCharIndex >= 0 && selectedCharIndex < characterPositions.size()) {
                selectedCharPos = characterPositions[selectedCharIndex];
                if (selectedCharIndex > 0 && selectedCharPos.x - activeMsgBorder.x == 0) {
                    selectedCharPos = characterPositions[selectedCharIndex-1];
                    selectedCharPos.x += renderer.text->getFont()->advance(console->activeMessage.back());
                }
            } else if (characterPositions.size() > 0 && selectedCharIndex == characterPositions.size()) {
                selectedCharPos = characterPositions.back();
                selectedCharPos.x += renderer.text->getFont()->advance(console->activeMessage.back());
            }
            
            activeMessageRect = addBorder(textRect, activeMsgBorder);
        }

        // render active message background
        // note that the max height of the text rect is calculated before padding, while the width is after padding
        activeMessageRect.w = MAX(activeMessageRect.w, activeMsgMinWidth);
        activeMessageRect.h = MAX(activeMessageRect.h, activeMsgMinHeight);
        renderer.colorRect(activeMessageRect, activeMessageBackground);
        
        // render cursor
        constexpr double flashDelay = 0.5;
        static double timeTilFlash = flashDelay;
        constexpr double flashDuration = 0.5;
        timeTilFlash -= Metadata->deltaTime / 1000.0; // subtract time in seconds from time
        double secondsSinceCursorMove = Metadata->seconds() - console->timeLastCursorMove;
        if (secondsSinceCursorMove < flashDuration) timeTilFlash = -secondsSinceCursorMove;
        if (timeTilFlash < 0.0) {
            FRect cursor = {
                selectedCharPos.x,
                selectedCharPos.y + renderer.text->getFont()->descender(),
                renderer.pixels(2.0f),
                renderer.text->lineHeight()
            };
            renderer.colorRect(cursor, GUI::Console::activeTextColor, 0.9f);
            if (timeTilFlash < -flashDuration) {
                timeTilFlash = flashDelay;
            }
        }
        
        logOffsetY = activeMessageRect.y + activeMessageRect.h;
    }

    if (console->promptOpen || Metadata->seconds() - console->timeLastMessageSent < CONSOLE_LOG_NEW_MESSAGE_OPEN_DURATION) {
        // Render console log
        SDL_Color logBackground = {90, 90, 90, 150};
        renderer.textBox(console->log, 10, console->logTextColors, {0, logOffsetY}, TextAlignment::BottomLeft, logBackground, renderer.pixels({20, 20}), {logMaxWidth, maxHeight});
    }
}

void renderFontComponents(glm::vec2 p, GuiRenderer& renderer) {
    auto* font = renderer.text->getFont();
    if (!font) return;
    auto* face = font->face;
    if (!face) return;

    auto height = face->height >> 6;
    auto ascender = face->ascender >> 6;;
    auto descender = face->descender >> 6;
    auto bbox = face->bbox;
    bbox.xMin >>= 6;
    bbox.xMax >>= 6;
    bbox.yMin >>= 6;
    bbox.yMax >>= 6;

    glm::vec3 points[] = {
        {p.x, p.y, 0},
        {p.x, p.y + descender, 0},
        {p.x, p.y + descender, 0},
        {p.x, p.y + descender + height, 0},
        {p.x, p.y + descender + height, 0},
        {p.x, p.y + descender + height + ascender, 0}
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
    //renderer.colorRect({addBorder(bounds, glm::vec2(0.0f)), {255, 0, 0, 100}});
    
    //Draw::thickLines(*renderer.quad, sizeof(points) / (2 * sizeof(glm::vec3)), points, colors, lineWidths);

    renderer.rectOutline(bounds, {0, 255, 0, 100}, 2.0f, 2.0f);
    renderer.colorRect(addBorder(bounds, glm::vec2(0.0f)), {255, 0, 0, 100});
}

static void testTextRendering(GuiRenderer& guiRenderer, TextRenderer& textRenderer) {
    
    auto rect3 = textRenderer.render("This should be centered\n right in the middle", guiRenderer.options.size / 2.0f,
        TextFormattingSettings(TextAlignment::MiddleCenter), TextRenderingSettings(glm::vec2{guiRenderer.textScale()}));
    
    auto rect = textRenderer.render("Bottom Right\nCorner!!p", {guiRenderer.options.size.x,0},
        TextFormattingSettings(TextAlignment::BottomRight), TextRenderingSettings(glm::vec2{guiRenderer.textScale() * 2}));
    //textRenderer.font->scale(3.0f);
   auto rect1 =  textRenderer.render("Top Right\nCorner!!\n mag font, min render scale", guiRenderer.options.size,
        TextFormattingSettings(TextAlignment::TopRight), TextRenderingSettings(glm::vec2{guiRenderer.textScale() / 3}));
    //textRenderer.font->scale(0.5f);
    auto rect2 = textRenderer.render("Top Left\nCorner!!\nmin font, mag render scale", {0,guiRenderer.options.size.y},
        TextFormattingSettings(TextAlignment::TopLeft), TextRenderingSettings(glm::vec2{guiRenderer.textScale() * 2}));
    //textRenderer.font->scale(1.0f);

    guiRenderer.rectOutline(rect.rect , {255, 0, 0, 255}, 1.0f, 2.0f);
    guiRenderer.rectOutline(rect1.rect, {255, 0, 0, 255}, 1.0f, 2.0f);
    guiRenderer.rectOutline(rect2.rect, {255, 0, 0, 255}, 1.0f, 2.0f);
    guiRenderer.rectOutline(rect3.rect, {255, 0, 0, 255}, 1.0f, 2.0f);
    

    renderFontComponents({100, 100}, guiRenderer);

    glm::vec3 points[4] = {
        {0, guiRenderer.options.size.y/2, 0},
        {guiRenderer.options.size.x, guiRenderer.options.size.y/2, 0},
        {guiRenderer.options.size.x/2, 0, 0},
        {guiRenderer.options.size.x/2, guiRenderer.options.size.y, 0},
    };
    Draw::thickLines(*guiRenderer.quad, 2, points, SDL_Color{0, 255, 0, 255}, 2.0f);
}

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

    gui->draw(guiRenderer, {0, 0, guiRenderer.options.size.x, guiRenderer.options.size.y}, playerControls.mousePixelPos(), &state->player, state->itemManager);

    //textRenderer.setFont(&ren.font);
    Draw::drawFpsCounter(textRenderer, (float)Metadata->fps(), guiRenderer.options);

    //textRenderer.setFont(&ren.debugFont);
    Draw::drawConsole(guiRenderer, &gui->console);
    //textRenderer.setFont(&ren.font);

    //testTextRendering(guiRenderer, textRenderer);

    guiRenderer.flush(ren.shaders, screenTransform);
    
}