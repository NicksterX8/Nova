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

constexpr int numChunkVerts = CHUNKSIZE * CHUNKSIZE * 1;

static void renderChunk(RenderContext& ren, Chunk* chunk, ChunkCoord pos, glm::vec2* vertexPositions, GLushort* vertexTexCoords) {
    assert(vertexPositions && vertexTexCoords);

    if (!chunk) return;
    
    const auto* atlas = &ren.textureAtlas;
    
    Vec2 startPos = pos * CHUNKSIZE;

    constexpr float tileWidth = 1.0f;
    constexpr float tileHeight = 1.0f;

    /*
    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = row * CHUNKSIZE + col;
            TileType type = (*chunk)[row][col].type;
            
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;

            TextureID texture = TileTypeData[type].background;
            auto texSpace = getTextureAtlasSpace(atlas, texture);

            const GLfloat positions[] = {
                x,  y
            };
            const GLushort texCoords[] = {
                texSpace.min.x, texSpace.min.y, texSpace.max.x, texSpace.max.y
            };
            
            // just in case padding or anything happens
            static_assert(sizeof(texCoords) == 4 * sizeof(GLushort), "incorrect number of floats in vertex data");
            static_assert(sizeof(positions) == 2 * sizeof(GLfloat), "incorrect number of floats in vertex data");

            memcpy(&vertexPositions[index], positions, sizeof(positions));
            memcpy(&vertexTexCoords[index * 4], texCoords, sizeof(texCoords));
        }
    }
    */

    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = row * CHUNKSIZE + col;
            float x = startPos.x + col * tileWidth;
            float y = startPos.y + row * tileHeight;
            vertexPositions[index] = {x, y};
        }
    }

    for (int row = 0; row < CHUNKSIZE; row++) {
        for (int col = 0; col < CHUNKSIZE; col++) {
            int index = (row * CHUNKSIZE + col) * 4;
            TileType type = (*chunk)[row][col].type;
            TextureID texture = TileTypeData[type].background;
            auto texSpace = getTextureAtlasSpace(atlas, texture);

            vertexTexCoords[index + 0] = texSpace.min.x;
            vertexTexCoords[index + 1] = texSpace.min.y;
            vertexTexCoords[index + 2] = texSpace.max.x;
            vertexTexCoords[index + 3] = texSpace.max.y;
        }
    }

}

int renderTilemap(RenderContext& ren, const Camera& camera, ChunkMap* chunkmap) {
    assert(isValidEntityPosition(camera.position));

    auto maxBoundingArea = camera.maxBoundingArea();
    Vec2 minChunkRelativePos = Vec2{(maxBoundingArea.x) / CHUNKSIZE, (maxBoundingArea.y) / CHUNKSIZE};
    Vec2 maxChunkRelativePos = Vec2{(maxBoundingArea.x + maxBoundingArea.w) / CHUNKSIZE, (maxBoundingArea.y + maxBoundingArea.h) / CHUNKSIZE};

    const IVec2 minChunkPos = {(int)floor(minChunkRelativePos.x), (int)floor(minChunkRelativePos.y)};
    const IVec2 maxChunkPos = {(int)floor(maxChunkRelativePos.x), (int)floor(maxChunkRelativePos.y)};
    const IVec2 viewportChunkSize = {maxChunkPos.x - minChunkPos.x, maxChunkPos.y - minChunkPos.y};

    struct ChunkPosPair {
        Chunk* chunk;
        IVec2 chunkCoord;
    };

    llvm::SmallVector<ChunkPosPair> chunks;

    ren.shaders.use(Shaders::Tilemap);

    int numRenderedChunks = 0;
    int drawCalls = 0;

    auto& chunkModel = ren.chunkModel;
    glBindVertexArray(chunkModel.vao);

    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
    glm::vec2* vertexPositions = (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
    GLushort* vertexTexCoords = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    
    for (int y = minChunkPos.y; y <= maxChunkPos.y; y++) {
        for (int x = minChunkPos.x; x <= maxChunkPos.x; x++) {
            if (!camera.rectIsVisible({x * CHUNKSIZE, y * CHUNKSIZE}, {(x+1) * CHUNKSIZE, (y+1) * CHUNKSIZE})) {
                continue;
            }
            ChunkData* chunkdata = chunkmap->get({x, y});
            if (!chunkdata) {
                ChunkData* newChunk = chunkmap->newChunkAt({x, y});
                if (newChunk) {
                    generateChunk(newChunk);
                    chunkdata = newChunk;
                } else {
                    LogError("Failed to create missing chunk at tile (%d,%d) for rendering", x*CHUNKSIZE, y*CHUNKSIZE);
                    // perhaps this should render some missing texture thing, but atleast for now just render nothing
                    continue;
                }
            }

            chunks.push_back({chunkdata->chunk, {x, y}});
        }
    }

    std::sort(chunks.begin(), chunks.end(), [](ChunkPosPair a, ChunkPosPair b){
        return a.chunk < b.chunk;
    });

    int chunksBuffered = 0;
    for (auto chunkAndPos : chunks) {
        if (chunksBuffered >= ChunkBuffer::bufferChunkCapacity) {
            // render a full batch of chunks
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
            glUnmapBuffer(GL_ARRAY_BUFFER);

            glDrawArrays(GL_POINTS, 0, chunksBuffered * numChunkVerts);

            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
            vertexPositions = (glm::vec2*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
            glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
            vertexTexCoords = (GLushort*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

            chunksBuffered = 0;
            drawCalls++;
        }

        size_t index = chunksBuffered;

        auto* chunkVertexPositions = vertexPositions + index * numChunkVerts; 
        auto* chunkVertexTexCoords = vertexTexCoords + index * numChunkVerts * 4;

        renderChunk(ren, chunkAndPos.chunk, chunkAndPos.chunkCoord, chunkVertexPositions, chunkVertexTexCoords);

        chunksBuffered++;
    }

    
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.positionVbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glBindBuffer(GL_ARRAY_BUFFER, chunkModel.texCoordVbo);
    glUnmapBuffer(GL_ARRAY_BUFFER);
    
    if (chunksBuffered > 0) {
        glDrawArrays(GL_POINTS, 0, chunksBuffered * numChunkVerts);
        chunksBuffered = 0;
        drawCalls++;
    }
    

    glBindVertexArray(0);

    return numRenderedChunks;
}

static GlModelSOA makeScreenModel() {
    constexpr int numVertices = 6;
    float z = 0.0f;
    const GLfloat vertexPositions[3 * numVertices] = {
        -1.0,   -1.0,   z, // bottom left
        -1.0,    1.0,   z, // top left
         1.0,    1.0,   z, // top right
         1.0,    1.0,   z, // top right
         1.0,   -1.0,   z, // bottom right
        -1.0,   -1.0,   z, // bottom left
    };
    const GLfloat vertexTexCoords[2 * numVertices] = {
        0.0f,  0.0f,
        0.0f,  1.0f,
        1.0f,  1.0f, 
        1.0f,  1.0f, 
        1.0f,  0.0f,
        0.0f,  0.0f
    };

    auto vertexFormat = GlMakeVertexFormat(0, {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {2, GL_FLOAT, sizeof(GLfloat)}, // velocity
    });

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, nullptr}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, vertexFormat);
    return model;
}

int setupShaders(RenderContext* ren) {
    auto& mgr = ren->shaders;

    auto entity = mgr.setup(Shaders::Entity, "entity");
    entity.setInt("texAtlas", TextureUnit::MyTextureAtlas);
    entity.setVec2("texAtlasSize", ren->textureAtlas.size);

    auto tilemap = mgr.setup(Shaders::Tilemap, "tilemap");
    tilemap.setInt("tex", TextureUnit::MyTextureAtlas);
    tilemap.setVec2("texSize", ren->textureAtlas.size);

    auto text = mgr.setup(Shaders::Text, "text");
    text.setInt("text", TextureUnit::Font0);

    auto sdf = mgr.setup(Shaders::SDF, "sdf");
    sdf.setInt("text", TextureUnit::Font1);
    sdf.setFloat("thickness", 0.5f);
    sdf.setFloat("soft", 0.2f);

    auto texture = mgr.setup(Shaders::Texture, "texture");
    texture.setInt("tex", TextureUnit::Random);

    auto quad = mgr.setup(Shaders::Quad, "color");
    quad.setInt("tex", TextureUnit::GuiAtlas);
    quad.setVec2("texSize", {10, 10});
    
    auto point = mgr.setup(Shaders::Point, "point");

    auto screen = mgr.setup(Shaders::Screen, "screen");
    screen.setInt("tex", TextureUnit::Screen);
    screen.setInt("numSamples", 6);
    screen.setVec2("velocity", {0, 0});

    GL::logErrors();

    return 0;
}

void renderInit(RenderContext& ren, int screenWidth, int screenHeight) {
    /* Init textures */
    ren.textures = TextureManager(TextureIDs::NumTextureSlots);
    setTextureMetadata(&ren.textures);
    ren.textureArray = makeTextureArray({256, 256}, &ren.textures, TextureTypes::World, FileSystem.assets.get(), TextureUnit::MyTextureArray);
    ren.textureAtlas = makeTextureAtlas(&ren.textures, TextureTypes::World, FileSystem.assets.get(), GL_NEAREST, GL_NEAREST, TextureUnit::MyTextureAtlas);

    /* Init Shaders */
    ren.shaders = {};
    setupShaders(&ren);
    
    /* Init text stuff */
    initFreetype();
    Font::FormattingSettings fontFormatting = {
        .lineSpacing = 1.0f,
        .tabSpaces = 5.0f
    };

    const char* fontPath = "fonts/Ubuntu-Regular.ttf";

    ren.font = Font(FileSystem.assets.get(fontPath), 48, true, 32, 127, SDL::pixelScale, fontFormatting, TextureUnit::Font0);
    ren.debugFont = Font(
        FileSystem.assets.get("fonts/Ubuntu-Regular.ttf"),
        24, // size
        false, // use sdfs?
        32, // first char  (space)
        127, // last char
        SDL::pixelScale, // scale
        fontFormatting, // formatting settings
        TextureUnit::Font1
    );
    ren.guiTextRenderer = TextRenderer::init(&ren.font);
    ren.guiTextRenderer.setFont(&ren.debugFont);

    ren.worldTextRenderer = TextRenderer::init(&ren.font);
    GL::logErrors();

    /* Init misc. renderers */
    ren.guiQuadRenderer = QuadRenderer(0);
    ren.worldQuadRenderer = QuadRenderer(0);

    TextureAtlas guiAtlas = makeTextureAtlas(&ren.textures, TextureTypes::Gui | TextureTypes::World, FileSystem.assets.get(), GL_LINEAR, GL_LINEAR, TextureUnit::GuiAtlas);
    RenderOptions guiOptions = {
        .size = {screenWidth, screenHeight}, // won't render if not set later
        .scale = 1.0f
    };
    ren.guiRenderer = GuiRenderer(&ren.guiQuadRenderer, &ren.guiTextRenderer, guiAtlas, guiOptions);
    RenderOptions worldGuiOptions = {
        .size = {INFINITY, INFINITY},
        .scale = BASE_UNIT_SCALE
    };
    ren.worldGuiRenderer = GuiRenderer(&ren.worldQuadRenderer, &ren.worldTextRenderer, guiAtlas, worldGuiOptions);
    GL::logErrors();

    /* Tilemap rendering setup */

    auto& chunkModel = ren.chunkModel;
    
    chunkModel.vao = setupVAO();

    auto vertCount = ChunkBuffer::bufferChunkCapacity * numChunkVerts;
    const auto positionVboFormat = GlMakeVertexFormat(0, {
        {2, GL_FLOAT, sizeof(GLfloat)},
    });
    chunkModel.positionVbo = setupVBO(positionVboFormat, vertCount, NULL, GL_STREAM_DRAW);

    const auto texCoordVboFormat = GlMakeVertexFormat(1, {
        {4, GL_UNSIGNED_SHORT, sizeof(GLushort)}
    });
    chunkModel.texCoordVbo = setupVBO(texCoordVboFormat, vertCount, NULL, GL_STREAM_DRAW);

    GL::logErrors();

    /* Set default opengl rendering options */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  
    glEnable(GL_PROGRAM_POINT_SIZE);

    ren.screenModel = makeScreenModel();
    ren.framebuffer = makeRenderBuffer({screenWidth, screenHeight});
    //resizeRenderBuffer(ren.framebuffer, {screenWidth, screenHeight});

    ren.renderSystem = new RenderSystem();
}

void renderQuit(RenderContext& ren) {
    /* Text rendering systems quit */
    ren.font.unload();
    ren.debugFont.unload();

    ren.guiTextRenderer.destroy();
    ren.guiQuadRenderer.destroy();
    ren.guiRenderer.destroy();
    ren.worldQuadRenderer.destroy();
    ren.worldTextRenderer.destroy();
    ren.worldGuiRenderer.destroy();

    quitFreetype();

    /* Shaders quit */
    ren.shaders.destroy();

    /* Textures quit */
    ren.textures.destroy();
    ren.textureArray.destroy();

    ren.chunkModel.destroy();
    ren.screenModel.destroy();

    destroyRenderBuffer(ren.framebuffer);
}

static llvm::SmallVector<ColorVertex, 0> makeDemoQuads() {
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
    if (!texture) return;

    glActiveTexture(GL_TEXTURE0 + TextureUnit::Random);
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

    auto vertexFormat = GlMakeVertexFormat(0, {
        {3, GL_FLOAT, sizeof(GLfloat)}, // position
        {2, GL_FLOAT, sizeof(GLfloat)}, // tex coord
        {1, GL_FLOAT, sizeof(GLfloat)} // rotation
    });

    const void* attributeVertices[] = {vertexPositions, vertexTexCoords, vertexRotations}; // order needs to be same as attribute order

    auto model = makeModelSOA(numVertices, attributeVertices, GL_STREAM_DRAW, vertexFormat);
    glBindVertexArray(model.model.vao);

    textureShader.use();

    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    model.destroy();
}

static void renderWorld(RenderContext& ren, Camera& camera, GameState* state, Vec2 playerTargetPos) {
    renderTilemap(ren, camera, &state->chunkmap);

    ren.renderSystem->Update(state->ecs, state->chunkmap, ren, camera);

    const static auto demoQuads = makeDemoQuads();
    ren.worldQuadRenderer.render(demoQuads);

    ren.worldGuiRenderer.colorRect({0.0f, 0.0f}, {1.0f, 1.0f}, {255, 0, 0, 255});
    ren.worldGuiRenderer.flush(ren.shaders, camera.getTransformMatrix());
}

RenderBuffer makeRenderBuffer(glm::ivec2 size) {
    /* Make fbo */
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    /* Make color attachment (texture) */
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x, size.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);  

    /* Make depth attachment with render buffer */
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, size.x, size.y);

    /* fin? */
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LogError("frame buffer not complete!");
    }

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    GL::logErrors();

    return RenderBuffer{fbo, rbo, texture};
}

void resizeRenderBuffer(RenderBuffer renderBuffer, glm::ivec2 newSize) {
    glActiveTexture(GL_TEXTURE0 + TextureUnit::Screen);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, newSize.x, newSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    //glBindRenderbuffer(GL_RENDERBUFFER, renderBuffer.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, newSize.x, newSize.y);
    glActiveTexture(GL_TEXTURE0);
}

void destroyRenderBuffer(RenderBuffer renderBuffer) {
    glDeleteRenderbuffers(1, &renderBuffer.rbo);
    glDeleteTextures(1, &renderBuffer.colorTexture);
    glDeleteFramebuffers(1, &renderBuffer.fbo);
}

void renderWorldRenderBuffer(RenderContext& ren, const Camera& camera) {
    glBindVertexArray(ren.screenModel.model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ren.screenModel.model.vbo);
    auto screenShader = ren.shaders.use(Shaders::Screen);

    glm::vec2 corners[4] = {
        camera.BottomLeftWorldPos(),
        camera.BottomRightWorldPos(),
        camera.TopRightWorldPos(),
        camera.TopLeftWorldPos()
    };

    static glm::vec2 oldCorners[4] = {
        corners[0], corners[1], corners[2], corners[3]
    };
    
    glm::vec2 velocities[4];
    for (int i = 0; i < 4; i++) {
        glm::vec2 pixelVelocity = glm::vec2(camera.worldToPixel(corners[i]) - camera.worldToPixel(oldCorners[i]));
        velocities[i] = glm::vec2(pixelVelocity.x / camera.pixelWidth, pixelVelocity.y / camera.pixelHeight);
    }

    memcpy(oldCorners, corners, sizeof(corners));

    glm::vec2 vertexVelocities[6] = {
        velocities[0], velocities[3], velocities[2],
        velocities[2], velocities[1], velocities[0]
    };

    size_t offset = ren.screenModel.attributeOffsets[2];
    glBufferSubData(GL_ARRAY_BUFFER, offset, sizeof(vertexVelocities), vertexVelocities);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render(RenderContext& ren, RenderOptions options, Gui* gui, GameState* state, Camera& camera, Vec2 playerTargetPos, Mode mode, bool doRenderWorld) {
    GL::logErrors();    
    auto& shaders = ren.shaders;

    assert(camera.worldScale() > 0.0f);

    const glm::mat4 screenTransform = glm::ortho(0.0f, (float)camera.pixelWidth, 0.0f, (float)camera.pixelHeight);
    const IVec2 screenMin = {0, 0}; (void)screenMin;
    const IVec2 screenMax = {camera.pixelWidth, camera.pixelHeight};
    
    const glm::mat4 worldTransform = camera.getTransformMatrix(); // or camTransform
    const Vec2 cameraMin = camera.minCorner();
    const Vec2 cameraMax = camera.maxCorner();
    const FRect maxBoundingArea = camera.maxBoundingArea();

    ren.shaders.use(Shaders::Quad).setMat4("transform", screenTransform);
    ren.shaders.use(Shaders::Tilemap).setMat4("transform", worldTransform);
    auto textureShader = ren.shaders.use(Shaders::Texture);
    textureShader.setMat4("transform", screenTransform);

    /* Start Rendering */
    /* First pass */
    if (true) {
        glBindFramebuffer(GL_FRAMEBUFFER, ren.framebuffer.fbo);

        glClearColor(0.5f, 0.5f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        renderWorld(ren, camera, state, playerTargetPos);

        if (getTick() == 3) {
            resizeRenderBuffer(ren.framebuffer, glm::ivec2(options.size.x + 1, options.size.y));
        }

        resizeRenderBuffer(ren.framebuffer, glm::ivec2(options.size.x, options.size.y + 1));
    }

    /* Second pass */
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // back to default

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // only need blending for points and stuff, not entities or tilemap
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    renderWorldRenderBuffer(ren, camera);

    GL::logErrors();

    auto quadShader = shaders.use(Shaders::Quad);
    quadShader.setMat4("transform", worldTransform);

    Draw::chunkBorders(ren.guiQuadRenderer, camera, SDL_Color{255, 0, 255, 125}, 8.0f, 0.5f);
    
    ren.shaders.use(Shaders::Quad).setMat4("transform", screenTransform);
    
    ren.guiQuadRenderer.flush(quadShader, worldTransform, ren.guiRenderer.guiAtlas.unit);
    
    GL::logErrors();

    // for text, depth mask off, dept

    // GL_BLEND is required for text, if text are boxes, that's probably why
    //glDisable(GL_BLEND);

    ren.guiRenderer.options = options;

    Draw::drawGui(ren, camera, screenTransform, gui, state);
    GL::logErrors();

    //renderTexture(ren.shaders.get(Shaders::Texture), ren.font.atlasTexture);

    if (mode == Testing) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
    }

    /* Done rendering */
    SDL_GL_SwapWindow(ren.window);

    GL::logErrors();
}