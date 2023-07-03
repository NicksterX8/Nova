#include "rendering/rendering.hpp"
#include "rendering/systems/simple.hpp"
#include "Chunks.hpp"
#include "utils/Debug.hpp"
#include "utils/Log.hpp"
#include "utils/random.hpp"
#include "My/Vec.hpp"
#include "utils/vectors.hpp"
#include "llvm/ArrayRef.h"
#include "global.hpp"
#include "rendering/drawing.hpp"

/*
void highlightTargetedEntity(SDL_Renderer* ren, float scale, const GameViewport* gameViewport, const Gui* gui, const GameState* state, Vec2 playerTargetPos) {
    Vec2 screenPos = gameViewport->worldToPixelPositionF(playerTargetPos.vfloor());
    // only draw tile marker if mouse is actually on the world, not on the Gui
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

constexpr int numChunkVerts = CHUNKSIZE * CHUNKSIZE * 4;
constexpr int numChunkFloats = numChunkVerts * sizeof(TilemapVertex) / sizeof(float);
constexpr int numChunkIndices = 6 * CHUNKSIZE * CHUNKSIZE;

static void renderChunk(RenderContext& ren, ChunkData& chunkdata, GlModel& chunkModel, GLfloat* vertexPositions, GLfloat* vertexTexCoords) {
    const TextureData* textureData = ren.textures.data.data;
    const glm::vec2 textureArraySize = glm::vec2(ren.textureArray.size);

    //glBindBuffer(GL_ARRAY_BUFFER, chunkModel.vbo);
    //void* vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    //GLfloat* vertexPositions = static_cast<GLfloat*>(vertexBuffer);
    //GLfloat* vertexTexCoords = vertexPositions + Render::numChunkVerts * 2; // 2 floats per vertex
    
    Vec2 startPos = chunkdata.tilePosition();

    const float tileWidth = 1.0f;
    const float tileHeight = 1.0f;

    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = row * CHUNKSIZE + col;
            TileType type = (*chunkdata.chunk)[row][col].type;
            if (type == TileTypes::Empty) continue;
            
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;
            float x2 = x + tileWidth;
            float y2 = y + tileHeight;

            TextureID texture = TileTypeData[type].background;
            glm::ivec2 textureSize = textureData[texture].size;
            glm::vec2 texMin = (glm::vec2(0.5f, 0.5f));
            glm::vec2 texMax = (glm::vec2(textureSize) - glm::vec2{0.5f, 0.5f});

            // make this SOA
            float tex = static_cast<float>(texture);
            const GLfloat texCoords[] = {
                texMin.x, texMin.y, tex,
                texMin.x, texMax.y, tex,
                texMax.x, texMax.y, tex,
                texMax.x, texMin.y, tex
            };
            const GLfloat positions[] = {
                x,  y,
                x,  y2,
                x2, y2,
                x2, y,
            };
            // just in case padding or anything happens
            static_assert(sizeof(texCoords) == 4 * sizeof(glm::vec3), "incorrect number of floats in vertex data");
            static_assert(sizeof(positions) == 4 * sizeof(glm::vec2), "incorrect number of floats in vertex data");

            memcpy(&vertexPositions[index * 8], positions, sizeof(positions));
            memcpy(&vertexTexCoords[index * 12], texCoords, sizeof(texCoords));
        }
    }

}

void renderTilemap(RenderContext& ren, const Camera& camera, ChunkMap* chunkmap) {
    auto maxBoundingArea = camera.maxBoundingArea();
    Vec2 minChunkRelativePos = Vec2{(maxBoundingArea.x) / CHUNKSIZE, (maxBoundingArea.y) / CHUNKSIZE};
    Vec2 maxChunkRelativePos = Vec2{(maxBoundingArea.x + maxBoundingArea.w) / CHUNKSIZE, (maxBoundingArea.y + maxBoundingArea.h) / CHUNKSIZE};

    const IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    const IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    const IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    auto& chunkModel = ren.chunkModel;
    glBindVertexArray(chunkModel.vao);
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunkModel.ebo);
    GL::logErrors();

    auto& chunkBuffer = ren.chunkBuffer;

    ren.shaders.use(Shaders::Tilemap);

    //My::Vec<int32_t> indicesToRender = My::Vec<int32_t>::Empty();

    // TRY USING SEPARATE VBO FOR NEW CHUNKS ??

    // check for stored chunk vertex stuff
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
            if (!chunkmap->existsAt({x, y})) {
                ChunkData* newChunk = chunkmap->newChunkAt({x, y});
                if (newChunk) {
                    generateChunk(newChunk);
                } else {
                    LogError("Failed to create missing chunk at tile (%d,%d) for rendering", x*CHUNKSIZE, y*CHUNKSIZE);
                    // perhaps this should render some missing texture thing, but atleast for now just render nothing
                    continue;
                }
            }

            /*
            auto* bufferIndex = chunkBuffer.map.lookup({x, y});
            if (bufferIndex) {
                // dont need to re generate vertices
                indicesToRender.push(*bufferIndex);
            }
            */
        }
    }

    int numRenderedChunks = 0;
    int drawCalls = 0;

    /*
    for (int i = 0; i < indicesToRender.size; i++) {
        int32_t index = indicesToRender[i];
        // offset to where the chunk's vertices are
        size_t offset = (size_t)index * numChunkIndices;
        glDrawElements(GL_TRIANGLES, numChunkIndices, GL_UNSIGNED_SHORT, (void*)offset);
        numRenderedChunks++;
    }
    */

    //chunkBuffer.map.destroy();
    //chunkBuffer.map = ChunkVertexMap::Empty();

    void* vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
            //Vec2 chunkMinPixel = camera.worldToPixel({x * CHUNKSIZE, y * CHUNKSIZE});
            //Vec2 chunkMaxPixel = camera.worldToPixel({(x+1) * CHUNKSIZE, (y+1) * CHUNKSIZE});
            //if (!camera.pixelOnScreen(chunkMinPixel) && !camera.pixelOnScreen(chunkMaxPixel)) continue;

            ChunkData* chunkdata = chunkmap->get({x, y});

            //auto* bufferIndex = chunkBuffer.map.lookup({x, y});
            if (/* !bufferIndex */ true) {
                if (chunkBuffer.chunksStored >= ChunkBuffer::bufferChunkCapacity) {
                    // render a full batch of chunks
                    glUnmapBuffer(GL_ARRAY_BUFFER);
                    glDrawElements(GL_TRIANGLES, chunkBuffer.chunksStored * numChunkIndices, GL_UNSIGNED_INT, (void*)0);
                    vertexBuffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
                    chunkBuffer.chunksStored = 0;
                    drawCalls++;
                }

                size_t index = chunkBuffer.chunksStored;

                GLfloat* vertexPositionsStart = (GLfloat*)vertexBuffer;
                GLfloat* vertexTexCoordsStart = vertexPositionsStart + (numChunkVerts * 2) * ChunkBuffer::bufferChunkCapacity; // 2 floats per vertex

                GLfloat* vertexPositions = vertexPositionsStart + index * numChunkVerts * 2; 
                GLfloat* vertexTexCoords = vertexTexCoordsStart + index * numChunkVerts * 3;

                renderChunk(ren, *chunkdata, chunkModel, vertexPositions, vertexTexCoords);

                chunkBuffer.chunksStored++;

                //chunkBuffer.map.update({x, y}, index);
            }
        }
    }

    glUnmapBuffer(GL_ARRAY_BUFFER);
    if (chunkBuffer.chunksStored > 0) {
        glDrawElements(GL_TRIANGLES, chunkBuffer.chunksStored * numChunkIndices, GL_UNSIGNED_INT, (void*)0);
        chunkBuffer.chunksStored = 0;
        drawCalls++;
    }

    LogInfo("Draw calls: %d", drawCalls);

    int numRenderedTiles = numRenderedChunks * CHUNKSIZE*CHUNKSIZE;
    //LogInfo("num rendered tiles: %d", numRenderedTiles);
    (void)numRenderedTiles;
}

static int loadShaders(RenderContext& ren) {
    auto& mgr = ren.shaders;

    auto entity = mgr.setup(Shaders::Entity, "entity");
    entity.setInt("texArray", TextureUnit::MyTextureArray);

    auto tilemap = mgr.setup(Shaders::Tilemap, "tilemap");
    tilemap.setInt("texArray", TextureUnit::MyTextureArray);

    auto text = mgr.setup(Shaders::Text, "text");
    text.setInt("text", TextureUnit::Text);

    auto sdf = mgr.setup(Shaders::SDF, "sdf");
    sdf.setInt("text", TextureUnit::Text);

    auto texture = mgr.setup(Shaders::Texture, "texture");
    texture.setInt("tex", TextureUnit::Single);

    auto color = mgr.setup(Shaders::Color, "color");
    
    auto point = mgr.setup(Shaders::Point, "point");

    return 0;
}

void renderInit(RenderContext& ren) {
    /* Init Shaders */
    ren.shaders = {};
    loadShaders(ren);
    GL::logErrors();

    /* Init textures */
    ren.textures = TextureManager(TextureIDs::NumTextureSlots);
    setTextureMetadata(&ren.textures);
    ren.textureArray = makeTextureArray({256, 256}, &ren.textures, FileSystem.assets.get());
    ren.shaders.use(Shaders::Entity).setVec2("texArraySize", ren.textureArray.size);
    ren.shaders.use(Shaders::Tilemap).setVec2("texArraySize", ren.textureArray.size);
    glActiveTexture(GL_TEXTURE0 + TextureUnit::MyTextureArray);
    glBindTexture(GL_TEXTURE_2D_ARRAY, ren.textureArray.texture);

    /* Init text stuff */
    initFreetype();
    ren.font = Font(6.0f, 2.0f, FileSystem.assets.get("fonts/FreeSans.ttf"), 128, SDL::pixelScale, false, 32, 127, TextureUnit::Text);
    ren.debugFont = Font(7.0f, 2.0f, FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"), 32, SDL::pixelScale, false, 32, 127, TextureUnit::Text);
    ren.textRenderer = TextRenderer::init(&ren.debugFont, ren.shaders.get(Shaders::Text), 0.0f);
    ren.textRenderer.defaultFormatting = TextFormattingSettings::Default();
    GL::logErrors();

    /* Init misc. renderers */
    ren.quadRenderer = QuadRenderer(ren.shaders.get(Shaders::Color));
    RenderOptions options = {
        .size = {0, 0}, // won't render if not set later
        .scale = 1.0f
    };
    ren.guiRenderer = GuiRenderer(&ren.quadRenderer, &ren.textRenderer, options);
    GL::logErrors();

    /* Tilemap rendering setup */
    constexpr GlVertexAttribute attributes[] = {
        {2, GL_FLOAT, sizeof(GLfloat)}, // pos
        {3, GL_FLOAT, sizeof(GLfloat)}, // tex coord
    };
    ren.chunkModel = makeModelIndexedSOA(
        ChunkBuffer::bufferChunkCapacity * numChunkVerts, NULL, GL_DYNAMIC_DRAW,
        ChunkBuffer::bufferChunkCapacity * numChunkIndices * sizeof(GLuint), NULL, GL_STATIC_DRAW,
        attributes, sizeof(attributes) / sizeof(GlVertexAttribute)
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ren.chunkModel.ebo);
    GLuint* chunkIndexBuffer = (GLuint*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    generateQuadVertexIndices(ChunkBuffer::bufferChunkCapacity * CHUNKSIZE * CHUNKSIZE, chunkIndexBuffer);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    GL::logErrors();

    /* Set default opengl rendering options */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);
}

void renderQuit(RenderContext& ren) {
    /* Text rendering systems quit */
    ren.font.unload();
    ren.debugFont.unload();
    ren.textRenderer.destroy();
    quitFreetype();

    /* Shaders quit */
    ren.shaders.destroy();

    /* Textures quit */
    ren.textures.destroy();
    ren.textureArray.destroy();
}



static llvm::SmallVector<ColorVertex, 0> makeDemoQuads(SDL_Window* window) {
    int drawableWidth,drawableHeight;
    SDL_GL_GetDrawableSize(window, &drawableWidth, &drawableHeight);

    auto quadPoints = llvm::SmallVector<ColorVertex, 0>();
    quadPoints.reserve(500 * 4);
 
    for (int i = 0; i < 500; i++) {
        Vec2 min = {randomInt(-100, 100), randomInt(-100, 100)};
        Vec2 max = min + Vec2{randomInt(1, 3), randomInt(1, 3)};
        
        quadPoints.push_back({glm::vec3(min.x, min.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(min.x, max.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(max.x, max.y, 0.0f), randomColor()});
        quadPoints.push_back({glm::vec3(max.x, min.y, 0.0f), randomColor()});
    }

    return quadPoints;
}

static void renderTexture(Shader textureShader, GLuint texture) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Single);
    glBindTexture(GL_TEXTURE_2D, texture);

    const auto p = glm::vec3{0, 0, 0.99f};
    const float w = 500;
    const float h = 500;
    const float rotation = 0.0f;

    constexpr int numVertices = 6;
    const GLfloat vertexPositions[3 * numVertices] = {
        p.x,     p.y,     p.z, // bottom left
        p.x,     p.y+h,   p.z, // top left
        p.x+w,   p.y+h,   p.z, // top right
        p.x+w,   p.y+h,   p.z, // top right
        p.x+w,   p.y,     p.z, // bottom right
        p.x,     p.y,     p.z  // bottom left
    };
    const GLfloat vertexTexCoords[2 * numVertices] = {
        0.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f, 
        1.0f,  1.0f, 
        1.0f,  0.0f,
        0.0f,  0.0f
    };
    const GLfloat vertexRotations[1 * numVertices] = {
        rotation,
        rotation,
        rotation,
        rotation,
        rotation,
        rotation
    };

    constexpr GlVertexAttribute attributes[] = {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {1, GL_FLOAT, sizeof(GLfloat)} // rotation
    };

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, vertexRotations}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, attributes, 3);
    glBindVertexArray(model.vao);

    textureShader.use();

    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    model.destroy();
}

void render(RenderContext& ren, RenderOptions options, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos, Mode mode) {
    GL::logErrors();    

    //LogInfo("drawable width: %d; camera pixel width: %f", drawableWidth, camera.pixelWidth);

    const glm::mat4 screenTransform = glm::ortho(0.0f, (float)camera.pixelWidth, 0.0f, (float)camera.pixelHeight);
    const IVec2 screenMin = {0, 0}; (void)screenMin;
    const IVec2 screenMax = {camera.pixelWidth, camera.pixelHeight};
    
    const glm::mat4 worldTransform = camera.getTransformMatrix(); // or camTransform
    const Vec2 cameraMin = camera.minCorner();
    const Vec2 cameraMax = camera.maxCorner();
    const FRect maxBoundingArea = camera.maxBoundingArea();

    ren.shaders.use(Shaders::Color).setMat4("transform", screenTransform);
    ren.shaders.use(Shaders::Tilemap).setMat4("transform", worldTransform);
    auto textureShader = ren.shaders.use(Shaders::Texture);
    textureShader.setMat4("transform", screenTransform);
    textureShader.setInt("tex", TextureUnit::Text);

    /* Start Rendering */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderTilemap(ren, camera, &state->chunkmap);

    static RenderSystem renderSystem = RenderSystem();
    renderSystem.Update(state->ecs, state->chunkmap, ren, camera);

    GL::logErrors();

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    auto quadShader = ren.quadRenderer.shader;
    quadShader.use();
    quadShader.setMat4("transform", worldTransform);
    Draw::chunkBorders(ren.quadRenderer, camera, SDL_Color{255, 0, 255, 125}, 8.0f, 0.5f);
    
    //static auto arr = makeDemoQuads(ren.window);
    //ren.quadRenderer.buffer(arr.size() / 4, arr.data());
    ColorQuadRenderBuffer::Vertex corners[] = {
        {{maxBoundingArea.x, maxBoundingArea.y, 0.0}, {255, 0, 0, 100}},
        {{maxBoundingArea.x, maxBoundingArea.y + maxBoundingArea.h, 0.0}, {0, 255, 0, 100}},
        {{maxBoundingArea.x + maxBoundingArea.w, maxBoundingArea.y + maxBoundingArea.h, 0.0}, {0, 0, 255, 100}},
        {{maxBoundingArea.x + maxBoundingArea.w, maxBoundingArea.y, 0.0}, {255, 255, 0, 100}}
    };
    //ren.quadRenderer.buffer(1, corners);
    auto pointShader = ren.shaders.use(Shaders::Point);
    Draw::ColoredPoint points[] = {
        {glm::vec3(camera.pixelToWorld({0, 0}), 0.9f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.pixelToWorld({camera.pixelWidth, 0}), 0.0f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.pixelToWorld({camera.pixelWidth, camera.pixelHeight- 20}), -0.9f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.pixelToWorld({0, camera.pixelHeight}), 0.9f), {255, 0, 0, 255}, 20.0f},
        {{200, 200, 0.95f}, {255, 0, 0, 255}, 20.0f},
    };
    pointShader.setMat4("transform", worldTransform);
    Draw::coloredPoints(sizeof(points) / sizeof(Draw::ColoredPoint), points);
    Draw::ColoredPoint points2[] = {
        {glm::vec3(camera.worldToPixel({0, 0}), 0.9f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.worldToPixel({10, 10}), 0.0f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.worldToPixel(camera.pixelToWorld(camera.pixelWidth, camera.pixelHeight)), -0.9f), {255, 0, 0, 255}, 10.0f},
        {glm::vec3(camera.worldToPixel({0, 20}), 0.9f), {255, 0, 0, 255}, 20.0f}
    };
    pointShader.setMat4("transform", screenTransform);
    Draw::coloredPoints(sizeof(points2) / sizeof(Draw::ColoredPoint), points2);
    ren.quadRenderer.flush();
    
    GL::logErrors();

    // for text, depth mask off, dept

    // GL_BLEND is required for text, if text are boxes, that's probably why
    //glDisable(GL_BLEND);

   // renderTexture(ren.shaders.get(Shaders::Texture), );

    ren.guiRenderer.options = options;

    Draw::drawGui(ren, camera, screenTransform, gui, state);

    //renderTexture(ren.shaders.get(Shaders::Texture), g.textTexture);

    if (mode == Testing) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
    }

    /* Done rendering */
    SDL_GL_SwapWindow(ren.window);

    GL::logErrors();
}