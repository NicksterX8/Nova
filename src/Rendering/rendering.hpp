#ifndef RENDERING_INCLUDED
#define RENDERING_INCLUDED

#include <vector>

#include <SDL2/SDL.h>
#include "../gl.h"
#include "../GameViewport.hpp"
#include "../GameState.hpp"
#include "../Tiles.hpp"
#include "../Metadata.hpp"
#include "../Textures.hpp"
#include "../Debug.hpp"
#include "../GUI.hpp"
#include "../NC/colors.h"
#include "../SDL2_gfx/SDL2_gfxPrimitives.h"
#include "../Log.hpp"

#include "Drawing.hpp"
#include "../EntitySystems/Rendering.hpp"
#include "Shader.hpp"
#include "../Camera.hpp"
#include "utils.hpp"

#include <glm/vec2.hpp>

SDL_Texture* screenTexture = NULL;

/*
int drawWorld(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, GameState* state) {
    int renWidth,renHeight;
    SDL_GetRendererOutputSize(ren, &renWidth, &renHeight);
    if (renWidth != gameViewport->display.w) {
        //Log("width is incorrect! renderwidth: %d, viewport width: %d", renWidth, gameViewport->display.w);
    }

    // draw space background
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderFillRect(ren, NULL);
    // stars

    Vec2 minChunkRelativePos = {gameViewport->world.x / CHUNKSIZE, gameViewport->world.y / CHUNKSIZE};
    Vec2 maxChunkRelativePos = {gameViewport->worldRightEdge() / CHUNKSIZE, gameViewport->worldBottomEdge() / CHUNKSIZE};
    IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);
    
    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
            const ChunkData* chunkdata = state->chunkmap.getChunkData({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.createChunk({x, y});
                generateChunk(newChunk->chunk);
                chunkdata = newChunk;
            }

            Vec2 screenDst = gameViewport->worldToPixelPositionF(Vec2(x * CHUNKSIZE, y * CHUNKSIZE));
            SDL_FRect destination = {
                screenDst.x,
                screenDst.y,
                TileWidth * CHUNKSIZE,
                TileHeight * CHUNKSIZE
            };
            
            // When zoomed out far, simplify rendering for better performance and less distraction
            if (TileWidth < 10.0f) {
                Draw::simpleChunk(chunkdata->chunk, ren, destination);
            } else {
                Draw::chunkExp(chunkdata->chunk, ren, scale, destination);
                // draw chunk coordinates
                if (Debug->settings.drawChunkCoordinates) {
                    // TODO: TEXT RENDER
                    //FC_DrawScale(FreeSans, ren, destination.x + 3*scale, destination.y + 2*scale, FC_MakeScale(0.5f,0.5f),
                    //"%d, %d", x * CHUNKSIZE, y * CHUNKSIZE);
                }

                if (Debug->settings.drawChunkEntityCount) {
                    // TODO: TEXT RENDER
                    //FC_DrawScale(FreeSans, ren, destination.x + destination.w/2.0f, destination.y + destination.h/2.0f, FC_MakeScale(0.5f,0.5f),
                    //"%llu", chunkdata->closeEntities.size());
                }
            }

        }
    }

    auto renderSystem = SimpleRectRenderSystem(ren, gameViewport);
    renderSystem.scale = scale;
    renderSystem.Update(state->ecs, state->chunkmap);

    using RenderSystemQuery = ECS::EntityQuery<
        ECS::RequireComponents<EC::Render, EC::Position, EC::Size>,
        ECS::AvoidComponents<>,
        ECS::LogicalOrComponents<>
    >;

    state->ecs.ForEach<RenderSystemQuery>([&](Entity entity){
        //Log("entity");
    });
    
    //if (Metadata.ticks() % 30 == 0)
    //    Log("rendered %d entities", nRenderedEntities);

    if (Debug->settings.drawChunkBorders) {
        Draw::drawChunkBorders(ren, scale, gameViewport);
    } 

    return numRenderedChunks;
}

int drawWorld2(RenderContext& ren, float scale, GameState* state, GameViewport* gameViewport) {
    Vec2 minChunkRelativePos = {gameViewport->world.x / CHUNKSIZE, gameViewport->world.y / CHUNKSIZE};
    Vec2 maxChunkRelativePos = {gameViewport->worldRightEdge() / CHUNKSIZE, gameViewport->worldBottomEdge() / CHUNKSIZE};
    IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    int numRenderedChunks = (viewportChunkSize.x+1) * (viewportChunkSize.y+1);

    for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
        for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
            const ChunkData* chunkdata = state->chunkmap.getChunkData({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.createChunk({x, y});
                generateChunk(newChunk->chunk);
                chunkdata = newChunk;
            }

            Vec2 screenDst = gameViewport->worldToPixelPositionF(Vec2(x * CHUNKSIZE, y * CHUNKSIZE));
            SDL_FRect destination = {
                screenDst.x,
                screenDst.y,
                TileWidth * CHUNKSIZE,
                TileHeight * CHUNKSIZE
            };
            
            // When zoomed out far, simplify rendering for better performance and less distraction
            if (TileWidth < 10.0f) {

                //Draw::simpleChunk(chunkdata->chunk, ren, destination);
            } else {
                //Draw::chunkExp(chunkdata->chunk, ren, scale, destination);
                // draw chunk coordinates
                if (Debug->settings.drawChunkCoordinates) {
                    //FC_DrawScale(FreeSans, ren, destination.x + 3*scale, destination.y + 2*scale, FC_MakeScale(0.5f,0.5f),
                    //"%d, %d", x * CHUNKSIZE, y * CHUNKSIZE);
                }

                if (Debug->settings.drawChunkEntityCount) {
                    //FC_DrawScale(FreeSans, ren, destination.x + destination.w/2.0f, destination.y + destination.h/2.0f, FC_MakeScale(0.5f,0.5f),
                    //"%llu", chunkdata->closeEntities.size());
                }
            }

        }
    }

    return numRenderedChunks;
}


void highlightTargetedTile(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const GUI* gui, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
        // only draw tile marker if mouse is actually on the world, not on the GUI
    if (!gui->pointInArea({(int)screenPos.x, (int)screenPos.y})) {
        SDL_FRect playerTileHover = {
            screenPos.x,
            screenPos.y,
            TileWidth,
            TileHeight
        };
        SDL_SetRenderDrawColor(ren, 0, 255, 255, 255);
        Draw::thickRect(ren, &playerTileHover, round(scale * 2));
    }      
}

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
    char path[512], vertexPath[512], fragmentPath[512];
    int shadersPathLen = strlen(FilePaths::shaders);
    memcpy(path, FilePaths::shaders, shadersPathLen);

    unsigned int nameLen = 0;
    char chr = name[0];
    while(chr != '\0') {
        path[nameLen+shadersPathLen] = chr;
        nameLen++;
        chr = name[nameLen];
    }

    int baseLen = shadersPathLen + nameLen;

    path[baseLen] = '.';
    path[baseLen+3] = '\0';

    size_t pathSize = baseLen + 4;
    memcpy(vertexPath, path, pathSize);
    memcpy(fragmentPath, path, pathSize);
    vertexPath[baseLen+1] = 'v';
    vertexPath[baseLen+2] = 's';
    fragmentPath[baseLen+1] = 'f';
    fragmentPath[baseLen+2] = 's';
 
    return Shader(vertexPath, fragmentPath);
}

int loadShaders(RenderContext& ren) {
    ren.entityShader = loadShader("entity");
    ren.tilemapShader = loadShader("tilemap");
    ren.pointShader = loadShader("point");
    ren.quadShader = loadShader("quad");
    
    return 0;
}

void renderInit(RenderContext& ren) {
    loadShaders(ren);
    ren.textureArray = makeTextureArray(FilePaths::assets);
    ren.tilemapShader.use();
    ren.tilemapShader.setInt("texArray", MY_TEXTURE_ARRAY_UNIT);
    ren.entityShader.use();
    ren.entityShader.setInt("texArray", MY_TEXTURE_ARRAY_UNIT);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);

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

    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    glActiveTexture(GL_TEXTURE0 + MY_TEXTURE_ARRAY_UNIT);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ren.textureArray);
}

void renderQuit(RenderContext& ren) {
    ren.entityShader.destroy();
    ren.tilemapShader.destroy();
    ren.pointShader.destroy();
    ren.chunkModel.destroy();
    glDeleteTextures(1, &ren.textureArray);
}

void render(RenderContext& ren, float scale, GUI* gui, GameState* state, Camera& camera, Vec2 playerTargetPos) {
    /* Start Rendering */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
            ChunkData* chunkdata = state->chunkmap.getChunkData({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = state->chunkmap.createChunk({x, y});
                generateChunk(newChunk->chunk);
                chunkdata = newChunk;
            }
            renderChunk(ren, *chunkdata, chunkModel);
        }
    }

    static RenderSystem renderSystem = RenderSystem();
    renderSystem.Update(state->ecs, state->chunkmap, ren, camera);

    std::vector<Draw::ColoredPoint> points;
    points.push_back({{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 0.5f}, 15.0f});
    auto r1 = camera.minCorner();
    auto r2 = camera.pixelToWorld({camera.pixelWidth/2.0f, camera.pixelHeight/2.0f});
    auto r3 = camera.maxCorner();
    points.push_back({glm::vec3(r1.x + 1, r1.y + 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push_back({glm::vec3(r3.x - 1, r3.y - 1, 0.0f), {0, 1, 0, 1}, 8.0f});
    points.push_back({glm::vec3(getMouseWorldPosition(camera), 0.0f), {0, 1, 1, 1}, 9.0f});

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    ren.pointShader.use();
    ren.pointShader.setMat4("transform", camTransform);
    Draw::coloredPoints(points.size(), &points[0]);

    ren.quadShader.use();
    ren.quadShader.setMat4("transform", camTransform);
    std::vector<Draw::ColorVertex> quadPoints{
        {{0, 0, 0.5}, {0, 1, 0, 1}},
        {{1, 0, 0.5}, {0, 1, 0, 1}},
        {{1, 1, 0.5}, {0, 1, 0, 1}},
        {{0, 1, 0.5}, {0, 1, 0, 1}},
        
        {{0, 0, 0.6}, {1, 0, 1, 1}},
        {{0.8, 0, 0.6}, {0, 1, 1, 1}},
        {{0.8, 0.8, 0.6}, {1, 1, 0, 1}},
        {{0, 0.8, 0.6}, {0, 1, 0, 1}}
    };
    Draw::coloredQuads(quadPoints.size() / 4, &quadPoints[0]);

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
    Draw::lines(1, linePoints, lineColors, lineWidths);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    SDL_GL_SwapWindow(ren.window);
}


#endif