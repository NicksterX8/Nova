#include "rendering/drawing.hpp"

/*
int Draw::drawChunkBorders(SDL_Renderer* renderer, float scale, const GameViewport* gameViewport) {
    float distanceToChunkX = fmod(gameViewport->world.x, CHUNKSIZE);
    float firstChunkX = gameViewport->world.x - distanceToChunkX;
    float distanceToChunkY = fmod(gameViewport->world.y, CHUNKSIZE);
    float firstChunkY = gameViewport->world.y - distanceToChunkY;
    Vec2 borderScreenPos = gameViewport->worldToPixelPositionF({firstChunkX, firstChunkY});
    int nLinesDrawn = 0;
    for (; borderScreenPos.x < gameViewport->displayRightEdge(); borderScreenPos.x += TileWidth * CHUNKSIZE) {
        debugVLine(renderer, borderScreenPos.x, gameViewport->display.y, gameViewport->displayBottomEdge(), scale);
        nLinesDrawn++;
    }
    for (; borderScreenPos.y < gameViewport->displayBottomEdge(); borderScreenPos.y += TileHeight * CHUNKSIZE) {
        // SDL_RenderDrawLineF(renderer, gameViewport->display.x, borderScreenPos.y, gameViewport->displayRightEdge(), borderScreenPos.y);
        debugHLine(renderer, borderScreenPos.y, gameViewport->display.x, gameViewport->displayRightEdge(), scale);
        nLinesDrawn++;
    }
    return nLinesDrawn;
}


int Draw::chunkBorders(float scale, Camera& camera) {
    const float z = 0.5f;
    float distanceToChunkX = fmod(camera.position.x, CHUNKSIZE);
    float firstChunkX = camera.position.x - distanceToChunkX;
    float distanceToChunkY = fmod(camera.position.y, CHUNKSIZE);
    float firstChunkY = camera.position.y - distanceToChunkY;
    Vec2 borderScreenPos = camera.worldToPixel({firstChunkX, firstChunkY});
    int nLinesDrawn = 0;
    int linesToDraw = ceil((camera.displayViewport.x + camera.displayViewport.w - borderScreenPos.x) / (TileWidth * CHUNKSIZE)) +
        ceil((camera.displayViewport.y + camera.displayViewport.h - borderScreenPos.y) / (TileHeight * CHUNKSIZE));
    glm::vec3* linePoints = new glm::vec3[linesToDraw*2];
    for (; borderScreenPos.x < camera.displayViewport.x + camera.displayViewport.w; borderScreenPos.x += TileWidth * CHUNKSIZE) {
        linePoints[nLinesDrawn * 2]   = {borderScreenPos.x, camera.displayViewport.y, z};
        linePoints[nLinesDrawn * 2+1] = {borderScreenPos.x, camera.displayViewport.y + camera.displayViewport.h, z};
        nLinesDrawn++;
    }
    for (; borderScreenPos.y < camera.displayViewport.y + camera.displayViewport.h; borderScreenPos.y += TileHeight * CHUNKSIZE) {
        linePoints[nLinesDrawn * 2]   = {borderScreenPos.y, camera.displayViewport.x, z};
        linePoints[nLinesDrawn * 2+1] = {borderScreenPos.y, camera.displayViewport.x + camera.displayViewport.w, z};
        nLinesDrawn++;
    }
    glm::vec4* lineColors = new glm::vec4[linesToDraw];
    GLfloat* lineWidths = new GLfloat[linesToDraw];
    for (GLuint i = 0; i < linesToDraw; i++) {
        lineColors[i] = glm::vec4(1, 1, 0, 0.5);
        lineWidths[i] = scale;
    }
    if (linesToDraw != nLinesDrawn) {
        printf("lines to draw is incorrect! linesToDraw: %d. nLinesDrawn: %d\n", linesToDraw, nLinesDrawn);
    }
    Draw::lines(nLinesDrawn, linePoints, lineColors, lineWidths);
    return nLinesDrawn;
}*/

int Draw::chunkBorders(ColorQuadRenderBuffer& renderer, const Camera& camera, SDL_Color color, float pixelLineWidth, float z) {
    glm::vec2 cameraMin = camera.minCorner();
    glm::vec2 cameraMax = camera.maxCorner();
    glm::ivec2 minChunkPos = toChunkPosition(cameraMin);
    glm::ivec2 maxChunkPos = toChunkPosition(cameraMax);

    

    // TODO: Rewrite this allowing camera rotation to exist

    /*

    int numLines = maxChunkPos.x - minChunkPos.x + maxChunkPos.y - minChunkPos.y + 2;
    int lineIndex = 0;

    assert(numLines >= 0);
    glm::vec3* points = new glm::vec3[numLines*2];
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        points[lineIndex*2] = glm::vec3(cameraMin.x, y * CHUNKSIZE, z);
        points[lineIndex*2+1] = glm::vec3(cameraMax.x, y * CHUNKSIZE, z);
        lineIndex++;
    }
    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        points[lineIndex*2] = glm::vec3(x * CHUNKSIZE, cameraMin.y, z);
        points[lineIndex*2+1] = glm::vec3(x * CHUNKSIZE, cameraMax.y, z);
        lineIndex++;
    }

    Draw::thickLines(renderer, numLines, points, color, pixelLineWidth / camera.scale());
    delete[] points;

    return numLines;
    */
    return 0;
}

void Draw::drawFpsCounter(TextRenderer& renderer, float fps, RenderOptions options) {
    static char fpsCounter[128];
    if (Metadata->ticks() % 10 == 0) {
        snprintf(fpsCounter, 128, "FPS: %.1f", (float)Metadata->fps());
    }
    
    renderer.render(fpsCounter, {0, options.size.y}, 
        TextFormattingSettings(TextAlignment::TopLeft), 
        TextRenderingSettings({255,0,0,255}, glm::vec2(options.scale * 2.0f)));
}

void Draw::drawConsole(GuiRenderer& renderer, const GUI::Console* console) {
    if (!console) return;

    // for everything
    float maxWidth = renderer.options.size.x;
    float maxHeight = renderer.options.size.y;
    // max width for log
    float logMaxWidth = MIN(150 * renderer.options.scale, maxWidth);

    glm::vec2 activeMsgBorder = glm::vec2{20.0f, 20.0f} * renderer.options.scale;

    // Render active message being written
    //renderer.text->font->lineHeightScale = 2.0f;
    TextFormattingSettings formatting(TextAlignment::BottomLeft, maxWidth); // active message max width is only the screen width
    TextRenderingSettings rendering({255, 255, 255, 255}, glm::vec2(renderer.options.scale));
    auto textRect = renderer.text->render(console->activeMessage.c_str(), activeMsgBorder, formatting, rendering);

    // render active message background
    float activeMsgMinWidth = logMaxWidth;
    float activeMsgMinHeight = renderer.text->lineHeight(rendering.scale.y); // always leave room for a line
    textRect.w = MAX(textRect.w, activeMsgMinWidth);
    textRect.h = MAX(textRect.h, activeMsgMinHeight);
    auto borderedTextRect = addBorder(textRect, activeMsgBorder);
    renderer.colorRect({borderedTextRect, {30, 30, 30, 205}});

    // Render console log
    SDL_Color logBackground = {90, 90, 90, 150};

    renderer.textBox({{0, borderedTextRect.y + borderedTextRect.h, logMaxWidth, maxHeight}, logBackground}, console->log, 3, console->logTextColors, 1.0f);
}

void Draw::drawGui(RenderContext& ren, const Camera& camera, const glm::mat4& screenTransform, GUI::Gui* gui, const GameState* state) {
    auto& textRenderer = ren.textRenderer;
    textRenderer.font = &ren.debugFont;
    textRenderer.z = 0.9f;

    GuiRenderer& guiRenderer = ren.guiRenderer;
    guiRenderer.quad->z = -0.9f;
    guiRenderer.text->z = 0.9f;

    auto colorShader = ren.quadRenderer.shader;
    colorShader.use();
    colorShader.setMat4("transform", screenTransform);

    auto textShader = ren.textRenderer.shader;
    textShader.use();
    textShader.setMat4("transform", screenTransform);

    Draw::drawFpsCounter(textRenderer, (float)Metadata->fps(), guiRenderer.options);
    if (gui->consoleOpen) {
        Draw::drawConsole(guiRenderer, &gui->console);
    }
    gui->draw(guiRenderer, 1.0f, {0, 0, (int)guiRenderer.options.size.x, (int)guiRenderer.options.size.y}, &state->player, state->itemManager);
    

    /*
    textRenderer.render("This should be centered\n right in the middle,\n on the dot!", guiRenderer.options.size / 2.0f,
        TextFormattingSettings(TextAlignment::MiddleCenter), TextRenderingSettings(glm::vec2{guiRenderer.options.scale}));
    textRenderer.render("Bottom Left\nCorner!!", {0,0},
        TextFormattingSettings(TextAlignment::BottomLeft), TextRenderingSettings(glm::vec2{guiRenderer.options.scale}));
    textRenderer.render("Bottom Right\nCorner!!", {guiRenderer.options.size.x,0},
        TextFormattingSettings(TextAlignment::BottomRight), TextRenderingSettings(glm::vec2{guiRenderer.options.scale}));
    textRenderer.render("Top Right\nCorner!!", guiRenderer.options.size,
        TextFormattingSettings(TextAlignment::TopRight), TextRenderingSettings(glm::vec2{guiRenderer.options.scale}));
    textRenderer.render("Top Left\nCorner!!", {0,guiRenderer.options.size.y},
        TextFormattingSettings(TextAlignment::TopLeft), TextRenderingSettings(glm::vec2{guiRenderer.options.scale}));
        */

    guiRenderer.flush(screenTransform, TextureUnit::Text);
}