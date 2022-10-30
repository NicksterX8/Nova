#include "rendering/Drawing.hpp"

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

int Draw::chunkBorders(ColorQuadRenderBuffer& renderer, Camera& camera, glm::vec4 color, float pixelLineWidth, float z) {
    glm::vec2 cameraMin = camera.minCorner();
    glm::vec2 cameraMax = camera.maxCorner();
    glm::ivec2 minChunkPos = toChunkPosition(cameraMin);
    glm::ivec2 maxChunkPos = toChunkPosition(cameraMax);

    int numLines = maxChunkPos.x - minChunkPos.x + maxChunkPos.y - minChunkPos.y + 2;
    int lineIndex = 0;

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

    Draw::thickLines(renderer, numLines, points, glm::vec4(1, 0, 1, 0.5f), pixelLineWidth / camera.scale());
    delete[] points;

    return numLines;
}