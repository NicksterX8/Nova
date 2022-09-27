#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <vector>

#include <SDL2/SDL.h>
#include "../sdl_gl.hpp"
#include "../utils/random.hpp"
#include "../GameState.hpp"
#include "../Tiles.hpp"
#include "../utils/Metadata.hpp"
#include "../Textures.hpp"
#include "../utils/Debug.hpp"
#include "../GUI/GUI.hpp"
#include "../SDL2_gfx/SDL2_gfxPrimitives.h"
#include "../utils/Log.hpp"

#include "Drawing.hpp"
#include "../EntitySystems/Rendering.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils.hpp"
#include "text.hpp"
#include "../My/Array.hpp"

#include <glm/vec2.hpp>

/*
void highlightTargetedEntity(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const GUI* gui, const GameState* state, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
    // only draw tile marker if mouse is actually on the world, not on the GUI
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        OptionalEntity<> targetedEntity = state->player.selectedEntity;
        if (targetedEntity.Exists(&state->ecs)) {
            if (targetedEntity.Has<EC::Render>(&state->ecs)) {
                SDL_FRect entityRect = targetedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, &entityRect, round(scale * 2));
            }
        } else {
            *
            forEachEntityInRange(&state->ecs, &state->chunkmap, playerTargetPos, M_SQRT2, [&](EntityT<EC::Position> entity){
                if (entity.Has<EC::Size, EC::Render>()) {
                    Vec2 position = *state->ecs.Get<EC::Position>(entity);
                    auto sizeComponent = state->ecs.Get<EC::Size>(entity);
                    Vec2 size = {sizeComponent->width, sizeComponent->height};
                    if (
                        playerTargetPos.x > position.x && playerTargetPos.x < position.x + size.x &&
                        playerTargetPos.y > position.y && playerTargetPos.y < position.y + size.y) {
                        // player is targeting this entity
                        Vec2 destPos = gameViewport->worldToPixelPositionF(position);
                        SDL_FRect entityRect = entity.Get<EC::Render>()->destination;
                        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                        Draw::thickRect(ren, &entityRect, round(scale * 2));
                        return 1;
                    }
                }
                return 0;
            });
            *
            auto focusedEntity = findPlayerFocusedEntity(&state->ecs, state->chunkmap, playerTargetPos);
            if (focusedEntity != NullEntity) {
                SDL_FRect* entityRect = &focusedEntity.Get<EC::Render>(&state->ecs)->destination;
                SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
                Draw::thickRect(ren, entityRect, round(scale * 2));
            }
        }
    }      
}
*/

struct TilemapVertex {
    glm::vec2 pos;
    glm::vec3 texCoord;
};

namespace Render {
    constexpr int numChunkVerts = CHUNKSIZE * CHUNKSIZE * 4;
    constexpr int numChunkFloats = numChunkVerts * sizeof(TilemapVertex) / sizeof(float);
    constexpr int numChunkIndices = 6 * CHUNKSIZE * CHUNKSIZE;
}

void renderChunk(RenderContext& ren, ChunkData& chunkdata, ModelData& chunkModel) {
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.VBO);
    void* vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    float* chunkVerts = static_cast<float*>(vertexBuffer);
    
    Vec2 startPos = chunkdata.tilePosition();

    const float tileWidth = 1.0f;
    const float tileHeight = 1.0f;

    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            unsigned int index = row * CHUNKSIZE + col;
            TileType type = (*chunkdata.chunk)[row][col].type;
            
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;
            float x2 = x + tileWidth;
            float y2 = y + tileHeight;

            TextureID texture = TileTypeData[type].background;
            float texMaxX = ((float)TextureData[texture].width)  / MY_TEXTURE_ARRAY_WIDTH;
            float texMaxY = ((float)TextureData[texture].height) / MY_TEXTURE_ARRAY_HEIGHT;
            if (texMaxX < 0.9f) {
                texMaxX -= 0.00001f;
            }
            if (texMaxY < 0.9f) {
                texMaxY -= 0.00001f;
            }
            float tex = static_cast<float>(texture);
            const float tileVerts[] = {
                x,  y,
                0.0f, 0.0f, tex,

                x,  y2,
                0.0f, texMaxY, tex,

                x2, y2,
                texMaxX, texMaxY, tex,

                x2, y,
                texMaxX, 0.0f, tex
            };
            static_assert(sizeof(tileVerts) == 4 * sizeof(TilemapVertex), "incorrect number of floats in vertex data");

            memcpy(&chunkVerts[index * 20], tileVerts, sizeof(tileVerts));
        }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);

    glDrawElements(GL_TRIANGLES, Render::numChunkIndices, GL_UNSIGNED_INT, 0);
}

Shader loadShader(const char* name) {
    return Shader(FileSystem.shaders.get(str_add(name, ".vs")), FileSystem.shaders.get(str_add(name, ".fs")));
}

int loadShaders(RenderContext& ren) {
    ren.entityShader = loadShader("entity");
    ren.tilemapShader = loadShader("tilemap");
    ren.pointShader = loadShader("point");
    ren.colorShader = loadShader("color");
    ren.textShader = loadShader("text");
    ren.sdfShader = loadShader("sdf");
    
    return 0;
}

void renderInit(RenderContext& ren) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    initFreetype();
    ren.font = new FontFace();
    *ren.font = loadFontFace(FileSystem.assets.get("fonts/FreeSans.ttf"), 64, true);
    ren.debugFont = new FontFace();
    *ren.debugFont = loadFontFace(FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"), 24, false);

    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);

    loadShaders(ren);
    ren.textureArray = makeTextureArray(FileSystem.assets.get());
    ren.tilemapShader.use();
    ren.tilemapShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.entityShader.use();
    ren.entityShader.setInt("texArray", TextureUnit::MyTextureArray);
    ren.textShader.use();
    ren.textShader.setInt("text", TextureUnit::Text);
    ren.sdfShader.use();
    ren.sdfShader.setInt("text", TextureUnit::Text);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);

    checkOpenGLErrors();

    const VertexAttribute attributes[] = {
        {2, GL_FLOAT, sizeof(GLfloat)},
        {3, GL_FLOAT, sizeof(GLfloat)}
    };
    ren.chunkModel = makeModel(
        Render::numChunkVerts * sizeof(TilemapVertex), NULL, GL_DYNAMIC_DRAW,
        Render::numChunkIndices * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW,
        attributes, sizeof(attributes) / sizeof(VertexAttribute)
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ren.chunkModel.EBO);
    GLuint* chunkIndexBuffer = static_cast<GLuint*>(glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY));

    for (GLuint i = 0; i < CHUNKSIZE*CHUNKSIZE; i++) {
        GLuint first = i*4;
        GLuint ind = i * 6;
        chunkIndexBuffer[ind+0] = first+0;
        chunkIndexBuffer[ind+1] = first+1;
        chunkIndexBuffer[ind+2] = first+3;
        chunkIndexBuffer[ind+3] = first+1;
        chunkIndexBuffer[ind+4] = first+2;
        chunkIndexBuffer[ind+5] = first+3; 
    }

    checkOpenGLErrors();

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    checkOpenGLErrors();
}

void renderQuit(RenderContext& ren) {
    doneFreetype();

    ren.entityShader.destroy();
    ren.tilemapShader.destroy();
    ren.pointShader.destroy();
    ren.textShader.destroy();
    ren.chunkModel.destroy();
    glDeleteTextures(1, &ren.textureArray);
}

My::Array<Draw::ColorVertex> makeDemoQuads(SDL_Window* window) {
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    auto quadPoints = My::Vec<Draw::ColorVertex>::FromList({
        {{200, 100, 0.5}, {0, 1, 0, 1}},
        {{200, 200, 0.5}, {0, 1, 0, 1}},
        {{400, 200, 0.5}, {0, 1, 0, 1}},
        {{400, 100, 0.5}, {0, 1, 0, 1}},
        
        {{0, drawableHeight, 0.6}, {1, 0, 1, 1}},
        {{50, drawableHeight, 0.6}, {0, 1, 1, 1}},
        {{50, drawableHeight-50, 0.6}, {1, 1, 0, 1}},
        {{0, drawableHeight-50, 0.6}, {0, 1, 0, 1}},

        {{2, 1, 0.5}, {0, 1, 0, 1}},
        {{2, 2, 0.5}, {0, 1, 1, 1}},
        {{4, 2, 0.5}, {1, 1, 0, 1}},
        {{4, 1, 0.5}, {0, 1, 0, 1}}
    });

    auto vec = My::Vec<int>(500 * 4);
 
    for (int i = 0; i < 500; i++) {
        Vec2 min = {randomInt(-100, 100), randomInt(-100, 100)};
        Vec2 max = min + Vec2{randomInt(1, 3), randomInt(1, 3)};
        quadPoints.push({glm::vec3(min.x, min.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push({glm::vec3(min.x, max.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push({glm::vec3(max.x, max.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
        quadPoints.push({glm::vec3(max.x, min.y, 0.0f), glm::vec4(randomInt(0, 1), randomInt(0, 1), randomInt(0, 1), randomInt(1, 10) / 10.0f)});
    }

    return quadPoints.asArray();
}

void render(RenderContext& ren, float scale, GUI* gui, GameState* state, Camera& camera, Vec2 playerTargetPos) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ren.textureArray);

    /* Start Rendering */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    checkOpenGLErrors();

    auto camTransform = camera.getTransformMatrix();
    ren.tilemapShader.use();
    ren.tilemapShader.setMat4("transform", camTransform);

    auto& chunkModel = ren.chunkModel;

    glBindVertexArray(chunkModel.VAO);

    Vec2 cameraMin = camera.minCorner();
    Vec2 cameraMax = camera.maxCorner();

    Vec2 minChunkRelativePos = {cameraMin.x / CHUNKSIZE, cameraMin.y / CHUNKSIZE};
    Vec2 maxChunkRelativePos = {cameraMax.x / CHUNKSIZE, cameraMax.y / CHUNKSIZE};
    IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);

    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
            ChunkData* chunkdata = state->chunkmap.get({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.newChunkAt({x, y});
                if (newChunk) {
                    generateChunk(newChunk->chunk);
                    chunkdata = newChunk;
                } else {
                    LogError("Failed to create missing chunk at tile (%d,%d) for rendering", x*CHUNKSIZE, y*CHUNKSIZE);
                    // perhaps this should render some missing texture thing, but atleast for now just render nothing
                    continue;
                }
                
            }
            renderChunk(ren, *chunkdata, chunkModel);
        }
    }

    int numRenderedTiles = numRenderedChunks * CHUNKSIZE*CHUNKSIZE;
    //LogInfo("num rendered tiles: %d", numRenderedTiles);

    static RenderSystem renderSystem = RenderSystem();
    renderSystem.Update(state->ecs, state->chunkmap, ren, camera);

    My::Vec<Draw::ColoredPoint> points(0);
    points.push({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}, 15.0f});
    auto r1 = camera.minCorner();
    auto r2 = camera.pixelToWorld({camera.pixelWidth/2.0f, camera.pixelHeight/2.0f});
    auto r3 = camera.maxCorner();
    points.push({glm::vec3(r1.x + 1, r1.y + 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push({glm::vec3(r3.x - 1, r3.y - 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push({glm::vec3(getMouseWorldPosition(camera), 0.0f), {0, 1, 1, 1}, 9.0f});

    static Draw::ColorQuadRenderBuffer quadRenderer = Draw::ColorQuadRenderBuffer();
    static auto quadPoints = makeDemoQuads(ren.window);

    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(ren.window, &drawableWidth, &drawableHeight);

    glm::mat4 quadTransform;
    glm::mat4 screenTransform = glm::ortho(0.0f, (float)drawableWidth, 0.0f, (float)drawableHeight);
    if (Debug->settings.drawChunkBorders) {
        quadTransform = screenTransform;
    } else {
        quadTransform = camTransform;
    }

    ren.colorShader.use();
    ren.colorShader.setMat4("transform", quadTransform);

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glDisable(GL_DEPTH_TEST);

    //ren.pointShader.use();
    //ren.pointShader.setMat4("transform", camTransform);
    //Draw::coloredPoints(points.size(), &points[0]);

    
    if (Debug->settings.drawEntityRects)
        Draw::coloredQuads(quadRenderer, quadPoints.size / 4, quadPoints.data);

    glm::vec3 linePoints[] = {
        {-1, -1, 0},
        {1, 1, 0}
    };
    glm::vec4 lineColors[] = {
        {1, 1, 0, 1}
    };
    GLfloat lineWidths[] = {
        0.01f
    };
    Draw::thickLines(quadRenderer, 1, linePoints, lineColors, lineWidths);

    Draw::chunkBorders(quadRenderer, camera, {1, 0, 0, 1}, 4.0f, 0.5f);

    SDL_FRect quadRect = {
        30, 30,
        400, 400
    };
    Draw::ColorQuadRenderBuffer::UniformQuad2D quadVerts = {
        {{quadRect.x, quadRect.y}, {quadRect.x+quadRect.w, quadRect.y}, {quadRect.x+quadRect.w, quadRect.y+quadRect.h}, {quadRect.x, quadRect.y+quadRect.h}},
        {0.5, 0.5, 0.5, 1.0}
    };
    quadRenderer.bufferUniform(1, &quadVerts, 1.0f);

    quadRenderer.flush();

    checkOpenGLErrors();

    glActiveTexture(GL_TEXTURE0 + TextureUnit::Text);

    auto textShader = ren.textShader;
    auto sdfShader = ren.sdfShader;
    textShader.use();
    textShader.setMat4("transform", screenTransform);

    sdfShader.use();
    sdfShader.setMat4("transform", screenTransform);
    sdfShader.setFloat("thickness", 0.5f);
    sdfShader.setFloat("soft", 0.01f);

    glDepthMask(GL_FALSE);
    renderText(ren.sdfShader, *ren.font, "hello\nworld!\nWhats up guys  \n\n!", glm::vec2{100, 100}, camera.scale() / 10, glm::vec3{1, 0, 0});
    renderText(ren.textShader, *ren.font, "Howdy\tpal!", glm::vec2(drawableWidth/2.0f, drawableHeight/2.0f), camera.scale() / 10, glm::vec3(0, 0, 0));
    renderText(ren.textShader, *ren.debugFont, "This is ubuntu with regular text!", glm::vec2{600, 500}, 1.0f, glm::vec3{0.0f, 0.0f, 0.0f});
    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    SDL_GL_SwapWindow(ren.window);

    checkOpenGLErrors();
}


#endif